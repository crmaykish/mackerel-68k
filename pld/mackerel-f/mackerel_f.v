// Mackerel-F Top Level SoC Module
module mackerel_f (
    input clk_27, // 27 MHz oscillator, pin 52
    input btn_rst_n, // Physical reset button pin 4
    
    output [5:0] led, // 6 onboard LEDs

    input uart_rx,
    output uart_tx
);

    // PLL: 27 MHz oscillator -> 48 MHz SoC clock
    wire clk_soc;
    wire pll_lock;
    pll soc_pll(.clk_in(clk_27), .clk_out(clk_soc), .locked(pll_lock));

    // Reset
    reg [23:0] rst = 24'd0;
    wire rst_cpu = ~rst[23];

    always @(posedge clk_soc) begin
        if (!btn_rst_n || !pll_lock) rst <= 24'd0;
        else if (rst_cpu) rst <= rst + 1'b1;
    end

    // Two-phase clock generation
    reg [1:0] phase = 2'b0;
    always @(posedge clk_soc) phase <= phase + 1'b1;
    wire enPhi1 = (phase == 2'b11);
    wire enPhi2 = (phase == 2'b01);

    // System Buses
    wire [23:1] ADDR_BUS;
    wire [15:0] DATA_BUS_OUT;
    reg [15:0] DATA_BUS_IN = 16'h0;

    // Control Lines
    wire RWn, ASn, LDSn, UDSn, FC0, FC1, FC2;
    wire DTACKn;

    // CPU Core
    fx68k cpu(
        .extReset(rst_cpu),
        .pwrUp(rst_cpu),
        .HALTn(1'b1),

        .clk(clk_soc),
        .enPhi1(enPhi1),
        .enPhi2(enPhi2),

        .eab(ADDR_BUS),
        .iEdb(DATA_BUS_IN),
        .oEdb(DATA_BUS_OUT),

        .eRWn(RWn),
        .ASn(ASn),
        .LDSn(LDSn),
        .UDSn(UDSn),

        .DTACKn(DTACKn),

        .FC0(FC0),
        .FC1(FC1),
        .FC2(FC2),

        .IPL0n(1'b1),
        .IPL1n(1'b1),
        .IPL2n(1'b1),

        .VPAn(1'b1),
        .BERRn(1'b1),
        .BRn(1'b1),
        .BGACKn(1'b1)
    );

    // BOOT Signal (Map ROM to 0x0000 temporarily on reset)
    wire BOOT;
    boot_signal bs1(~rst_cpu, ASn, BOOT);

    // Memory Map
    //  RAM          0x000000-0xFF7FFF  ~16 MB
    //  ROM          0xFF8000-0xFFF7FF  30 KB
    //  Peripherals  0xFFF800-0xFFFFFF  2 KB    8 slots x 256 B:
    //                 slot 0  0xFFF800  GPIO
    //                 slot 1  0xFFF900  UART
    //                 slot 2-7          reserved
    wire [23:0] address = {ADDR_BUS, 1'b0};   // ADDR_BUS has no bit 0, add it for convenient math

    wire in_periph = &address[23:11];                 // top 2 KB
    wire in_rom    = (&address[23:15]) && ~in_periph;  // top 32 KB minus top 2 KB
    wire in_ram    = ~(&address[23:15]);               // everything below 0xFF8000
    wire [2:0] periph_sel = address[10:8];             // 1 of 8 peripheral slots

    // Boot shadow maps ROM into low memory until BOOT asserts, so the CPU fetches
    // the reset vector from ROM at 0x0; afterward RAM owns 0x0.
    wire cs_rom_n    = ~(~ASn && (~BOOT || in_rom));
    wire cs_ram_n    = ~(~ASn &&  BOOT && in_ram);
    wire cs_periph_n = ~(~ASn &&  BOOT && in_periph);

    // TODO: Decode FC lines to avoid conflicting with IACK

    wire cs_gpio_n = ~(~cs_periph_n && periph_sel == 3'd0);
    wire cs_uart_n = ~(~cs_periph_n && periph_sel == 3'd1);

    // ROM: 1K x 16 = 2 KB physical, mirrored across the 30 KB ROM region
    reg [15:0] rom [0:1023];
    reg [15:0] rom_out;
    // Preload the ROM with a hex file
    initial $readmemh("rom.hex", rom);
    always @(posedge clk_soc) rom_out <= rom[ADDR_BUS[10:1]];

    // RAM: 4K x 16 = 8 KB, mirrored across the RAM region (low 13 addr bits only)
    reg [15:0] sram [0:4095];
    reg [15:0] sram_out;
    always @(posedge clk_soc) begin
        if (~cs_ram_n) begin
            if (~RWn && ~UDSn) sram[ADDR_BUS[12:1]][15:8] <= DATA_BUS_OUT[15:8];
            if (~RWn && ~LDSn) sram[ADDR_BUS[12:1]][7:0] <= DATA_BUS_OUT[7:0];
            sram_out <= sram[ADDR_BUS[12:1]];
        end
    end

    // GPIO
    reg [7:0] gpio = 8'b0;
    always @(posedge clk_soc) begin
        // GPIO writes
        if (~cs_gpio_n && ~RWn && ~UDSn) gpio <= DATA_BUS_OUT[15:8];
    end

    // UART
    wire irq_uart;
    wire dtack_uart;
    wire [7:0] uart_dout;

    uart u1(
        .clk(clk_soc),
        .rst_n(~rst_cpu),

        .cs_n(cs_uart_n),
        .reg_addr(ADDR_BUS[3:1]),
        .rwn(RWn),
        .ds_n(UDSn),
        .data_in(DATA_BUS_OUT[15:8]),
        .data_out(uart_dout),
        .dtack_n(dtack_uart),
        .irq(irq_uart),

        .rx(uart_rx),
        .tx(uart_tx)
    );

    // DTACK Generation
    assign DTACKn = cs_uart_n ? 1'b0 : dtack_uart;  // DTACK grounded unless UART is selected

    // Databus Input Mux - map the correct memory/peripheral data bus to the CPU on read cycles
    always @(*) begin
        if (~cs_rom_n) DATA_BUS_IN = rom_out;
        else if (~cs_ram_n) DATA_BUS_IN = sram_out;
        else if (~cs_gpio_n) DATA_BUS_IN = {gpio, 8'h00};       // Pad 8-bit peripherals with LDS of 0
        else if (~cs_uart_n) DATA_BUS_IN = {uart_dout, 8'h00};  // "
        else DATA_BUS_IN = 16'h0000;
    end

    // Debug LEDs (active low)
    assign led = ~gpio[5:0];

endmodule
