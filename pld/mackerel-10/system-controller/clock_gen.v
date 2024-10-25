module clock_gen(
    input CLK_IN,
    output CLK_OUT
);

// Generate CPU clock from source oscillator
reg [1:0] clk_buf = 0;
assign CLK_OUT = clk_buf[0];	// Divide source clock by 2 to get CPU clock
//assign CLK_OUT = CLK_IN;    // Pass the source oscillator through to CPU clock
always @(posedge CLK_IN) clk_buf <= clk_buf + 1'b1;

endmodule
