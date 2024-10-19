// Priority encode 7 interrupt sources (active high) to three interrupt priority level pins (active low)

module irq_encoder(
	input irq1,
	input irq2,
	input irq3,
	input irq4,
	input irq5,
	input irq6,
	input irq7,
	output reg ipl0_n,
	output reg ipl1_n,
	output reg ipl2_n
);

always @(*) begin
	if (irq7) begin
		ipl0_n = 0;
		ipl1_n = 0;
		ipl2_n = 0;
	end
	else if (irq6) begin
		ipl0_n = 1;
		ipl1_n = 0;
		ipl2_n = 0;
	end
	else if (irq5) begin
		ipl0_n = 0;
		ipl1_n = 1;
		ipl2_n = 0;
	end
	else if (irq4) begin
		ipl0_n = 1;
		ipl1_n = 1;
		ipl2_n = 0;
	end
	else if (irq3) begin
		ipl0_n = 0;
		ipl1_n = 0;
		ipl2_n = 1;
	end
	else if (irq2) begin
		ipl0_n = 1;
		ipl1_n = 0;
		ipl2_n = 1;
	end
	else if (irq1) begin
		ipl0_n = 0;
		ipl1_n = 1;
		ipl2_n = 1;
	end
	else begin
		ipl0_n = 1;
		ipl1_n = 1;
		ipl2_n = 1;
	end
end

endmodule
