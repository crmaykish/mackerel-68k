// Mackerel-F Top Level SoC Module
module mackerel_f (
    input sys_clk,       // 27 MHz oscillator, pin 52

    input btn_rst_n,     // Physical reset button pin 4

    output [5:0] led       // 6 onboard LEDs
);

    // Reset
    reg [23:0] rst = 24'd0;
    wire rst_cpu = ~rst[23];

    always @(posedge sys_clk) begin
        if (!btn_rst_n) rst <= 24'd0;
        else if (rst_cpu) rst <= rst + 1'b1;
    end

    // Two-phase clock generation
    reg [1:0] phase = 2'b0;
    always @(posedge sys_clk) phase <= phase + 1'b1;
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

        .clk(sys_clk),
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

    // ROM: 1K x 16 = 2KB
    reg [15:0] rom [0:1024];
    reg [15:0] rom_out;
    // Preload the ROM with a hex file
    initial $readmemh("rom.hex", rom);
    // Connect the address bus between the CPU and ROM
    always @(posedge sys_clk) rom_out <= rom[ADDR_BUS[10:1]];

    // Memory Map
    wire [23:0] address = {ADDR_BUS, 1'b0};    // ADDR_BUS does not have a 0th bit, add it in for convenient math
    wire cs_rom_n = ~(~ASn && (~BOOT || (address >= 24'hF00000)));
    wire cs_sram_n = 1'b1;
    wire cs_gpio_n = 1'b1;
    wire cs_uart_n = 1'b1;

    // DTACK Generation
    assign DTACKn = 1'b0;

    // Data Mux - map the correct memory/peripheral data bus to the CPU on read cycles
    always @(*) begin
        // When ROM is selected: ROM databus OUT -> CPU databus IN
        if (~cs_rom_n) DATA_BUS_IN = rom_out;
        // Nothing selected - hold CPU data bus LOW
        else DATA_BUS_IN = 16'h0000;
    end

    // Debug LEDs
    assign led[0] = ~rst_cpu;

    assign led[2] = ~ADDR_BUS[20];
    assign led[3] = ~ADDR_BUS[21];
    assign led[4] = ~ADDR_BUS[22];
    assign led[5] = ~ADDR_BUS[23];

endmodule
