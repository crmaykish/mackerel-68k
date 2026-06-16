// Mackerel-F Top Level SoC Module
module mackerel_f (
    input clk_27, // 27 MHz oscillator, pin 4

    output [5:0] led, // 6 onboard LEDs

    input uart_rx,
    output uart_tx,

    // SDRAM (Tang Nano 20k in-package SiP; pins auto-routed by name, no .cst)
    output O_sdram_clk,
    output O_sdram_cke,
    output O_sdram_cs_n,
    output O_sdram_cas_n,
    output O_sdram_ras_n,
    output O_sdram_wen_n,
    output [3:0]  O_sdram_dqm,
    output [10:0] O_sdram_addr,
    output [1:0]  O_sdram_ba,
    inout  [31:0] IO_sdram_dq
);

    // PLL: 27 MHz oscillator -> 64.8 MHz SoC clock (+ phase-shifted SDRAM chip clock)
    wire clk_soc;
    wire clk_soc_ps;
    wire pll_lock;
    pll soc_pll(.clk_in(clk_27), .clk_out(clk_soc), .clk_out_ps(clk_soc_ps), .locked(pll_lock));

    // Reset
    reg [23:0] rst = 24'd0;
    wire rst_cpu = ~rst[23];

    always @(posedge clk_soc) begin
        if (!pll_lock) rst <= 24'd0;   // power-on reset, held until the PLL locks
        else if (rst_cpu) rst <= rst + 1'b1;
    end

    // Two-phase clock: fx68k enables on alternate clk_soc cycles -> CPU = clk_soc/2 = 32.4 MHz
    reg phase = 1'b0;
    always @(posedge clk_soc) phase <= ~phase;
    wire enPhi1 = (phase == 1'b1);
    wire enPhi2 = (phase == 1'b0);

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
    boot_signal bs(~rst_cpu, ASn, BOOT);

    // Memory Map
    //  SDRAM        0x000000-0x7FFFFF  8 MB system RAM (sdram.v adapter + sdram_nano.v controller)
    //  (unmapped)   0x800000-0xFF7FFF  -- nothing here (no DTACK; a stray access would hang)
    //  ROM          0xFF8000-0xFFF7FF  30 KB
    //  Peripherals  0xFFF800-0xFFFFFF  2 KB    8 slots x 256 B:
    //                 slot 0  0xFFF800  GPIO
    //                 slot 1  0xFFF900  UART
    //                 slot 2-7          reserved
    wire [23:0] address = {ADDR_BUS, 1'b0};   // ADDR_BUS has no bit 0, add it for convenient math

    wire in_periph = &address[23:11];                 // top 2 KB
    wire in_rom    = (&address[23:15]) && ~in_periph;  // top 32 KB minus top 2 KB
    wire in_ram    = ~(&address[23:15]);               // everything below 0xFF8000
    wire in_sdram  = in_ram && ~address[23];           // low 8 MB (0x000000-0x7FFFFF) -> SDRAM system RAM
    wire [2:0] periph_sel = address[10:8];             // 1 of 8 peripheral slots

    // Boot shadow maps ROM into low memory until BOOT asserts, so the CPU fetches
    // the reset vector from ROM at 0x0; afterward SDRAM owns 0x0.
    wire cs_rom_n    = ~(~ASn && (~BOOT || in_rom));
    wire cs_sdram_n  = ~(~ASn &&  BOOT && in_sdram);
    wire cs_periph_n = ~(~ASn &&  BOOT && in_periph);

    // TODO: Decode FC lines to avoid conflicting with IACK

    wire cs_gpio_n = ~(~cs_periph_n && periph_sel == 3'd0);
    wire cs_uart_n = ~(~cs_periph_n && periph_sel == 3'd1);

    // ROM: 16K x 16 = 32 KB physical, covering the whole 30 KB ROM region
    // (linear, no longer mirrored -- ADDR_BUS[14:1] is the in-region word index).
    reg [15:0] rom [0:16383];
    reg [15:0] rom_out;
    // Preload the ROM with a hex file
    initial $readmemh("rom.hex", rom);
    always @(posedge clk_soc) rom_out <= rom[ADDR_BUS[14:1]];


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

    uart u(
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

    // SDRAM (8 MB, in-package SDRAM via the sdram.v adapter + sdram_nano.v controller)
    wire [15:0] sdram_dout;
    wire        dtack_sdram;

    sdram_controller s(
        .clk(clk_soc),
        .clk_ps(clk_soc_ps),
        .rst_n(pll_lock),          // inits independently of the CPU reset counter

        .cs_n(cs_sdram_n),
        .addr(ADDR_BUS[22:1]),
        .rwn(RWn),
        .udsn(UDSn),
        .ldsn(LDSn),
        .wdata(DATA_BUS_OUT),
        .rdata(sdram_dout),
        .dtack_n(dtack_sdram),
        .init_done(),              // reserved: gate reset until SDRAM init done (when SDRAM is system RAM)

        .O_sdram_clk(O_sdram_clk),
        .O_sdram_cke(O_sdram_cke),
        .O_sdram_cs_n(O_sdram_cs_n),
        .O_sdram_cas_n(O_sdram_cas_n),
        .O_sdram_ras_n(O_sdram_ras_n),
        .O_sdram_wen_n(O_sdram_wen_n),
        .O_sdram_dqm(O_sdram_dqm),
        .O_sdram_addr(O_sdram_addr),
        .O_sdram_ba(O_sdram_ba),
        .IO_sdram_dq(IO_sdram_dq)
    );

    // DTACK Generation -- 0 (no wait) for ROM/RAM/GPIO; from the peripheral/SDRAM otherwise
    assign DTACKn = ~cs_sdram_n ? dtack_sdram :
                    ~cs_uart_n  ? dtack_uart  :
                                  1'b0;

    // Databus Input Mux - map the correct memory/peripheral data bus to the CPU on read cycles
    always @(*) begin
        if (~cs_rom_n) DATA_BUS_IN = rom_out;
        else if (~cs_sdram_n) DATA_BUS_IN = sdram_dout;
        else if (~cs_gpio_n) DATA_BUS_IN = {gpio, 8'h00};       // Pad 8-bit peripherals with LDS of 0
        else if (~cs_uart_n) DATA_BUS_IN = {uart_dout, 8'h00};  // "
        else DATA_BUS_IN = 16'h0000;
    end

    // Debug LEDs (active low)
    assign led = ~gpio[5:0];

endmodule
