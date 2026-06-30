module clock_gen(
    input CLK_IN,
    output CLK_OUT
);

// 0: Run the CPU clock at the full oscillator frequency
// 1: Run the CPU clock at half the oscillator frequency
parameter DIV2 = 0;

reg clk_buf = 1'b0;
always @(posedge CLK_IN) clk_buf <= clk_buf + 1'b1;
assign CLK_OUT = DIV2 ? clk_buf : CLK_IN;

endmodule
