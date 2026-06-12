// Mackerel-F Top Level SoC Module
module mackerel_f (
    input sys_clk,       // 27 MHz oscillator, pin 52

    input btn_rst_n,     // Physical reset button pin 4

    output [5:0] led       // 6 onboard LEDs
);

    // reset
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

    // Main 68000 CPU Core
    wire [23:1] ADDR_BUS;
    wire [15:0] DATA_BUS_IN;
    wire [15:0] DATA_BUS_OUT;

    fx68k cpu(
        .extReset(rst_cpu),
        .pwrUp(rst_cpu),
        .HALTn(1'b1),

        .clk(sys_clk),
        .enPhi1(enPhi1),
        .enPhi2(enPhi2),

        .eab(ADDR_BUS),
        .iEdb(16'b0),
        .oEdb(DATA_BUS_OUT),

        .DTACKn(1'b0),

        .IPL0n(1'b1),
        .IPL1n(1'b1),
        .IPL2n(1'b1),

        .VPAn(1'b1),
        .BERRn(1'b1),
        .BRn(1'b1),
        .BGACKn(1'b1)
    );

    // Debug LEDs
    assign led[0] = ~rst_cpu;

    assign led[2] = ~ADDR_BUS[20];
    assign led[3] = ~ADDR_BUS[21];
    assign led[4] = ~ADDR_BUS[22];
    assign led[5] = ~ADDR_BUS[23];

endmodule
