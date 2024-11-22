module clock_gen(
    input CLK_IN,
    output CLK_OUT
);

// Generate CPU clock from source oscillator
reg clk_buf = 1'b0;
assign CLK_OUT = clk_buf;	// Divide source clock by 2 to get CPU clock
always @(posedge CLK_IN) clk_buf <= clk_buf + 1'b1;

endmodule
