module system_controller(
	input CLK,
	input RST,
	
	output CLK_CPU,
	output reg [2:0] LED,
	
	output IPL0, IPL1, IPL2,
	
	output BERR, DTACK, VPA,
	
	input [7:0] DATA,
	
	input [23:14] ADDR_H,
	input [4:1] ADDR_L,
	
	input AS, UDS, LDS,
	
	input RW,
	
	input FC0, FC1, FC2,
	
	output ROM_LOWER, ROM_UPPER,
	output RAM_LOWER, RAM_UPPER,
	output DUART,
	output EXP,
	
	output IACK_DUART,
	
	output reg [7:0] GPIO
	
);

assign IACK_DUART = 1;
assign EXP = 1;

assign DTACK = 0;
assign BERR = 1;
assign VPA = 1;

assign IPL0 = 1;
assign IPL1 = 1;
assign IPL2 = 1;

// Generate BOOT signal for first four bus cycles after reset
/*
reg BOOT = 1'b0;
reg [2:0] bus_cycles = 0;

always @(posedge AS) begin
	if (~RST) begin 
		bus_cycles = 0;
		BOOT <= 1'b0;
	end
	else begin
		if (~BOOT) begin
			bus_cycles <= bus_cycles + 4'b1;
			if (bus_cycles == 4'd4) BOOT <= 1'b1;
		end
	end
end
*/

// Generate CPU clock from oscillator
reg [2:0] clk_buf = 0;
assign CLK_CPU = clk_buf[0];
always @(posedge CLK) clk_buf <= clk_buf + 3'b1;

// Handle memory addressable GPIO on CPLD
always @(posedge CLK_CPU) begin
	if (~RST)
		begin
			LED <= 0;
			GPIO <= 0;
		end
	else
		// LED at 0xF00001
		if (ADDR_H[23] && ADDR_H[22] && ADDR_H[21] && ADDR_H[20] && ~ADDR_L[1]) begin
			if (~LDS && ~RW) LED <= DATA[2:0];
		end
		
		// GPIO at 0xF00003
		if (ADDR_H[23] && ADDR_H[22] && ADDR_H[21] && ADDR_H[20] && ADDR_L[1]) begin
			if (~LDS && ~RW) GPIO <= DATA[7:0];
		end
end

// ROM enabled at 0x000000 - 0x100000
wire ROM_EN = ~ADDR_H[23] && ~ADDR_H[22] && ~ADDR_H[21] && ~ADDR_H[20];
assign ROM_LOWER = ~(~AS && ~LDS && ROM_EN);
assign ROM_UPPER = ~(~AS && ~UDS && ROM_EN);

// SRAM enabled at 0x800000 - 0x900000
wire RAM_EN = ADDR_H[23] && ~ADDR_H[22] && ~ADDR_H[21] && ~ADDR_H[20];
assign RAM_LOWER = ~(~AS && ~LDS && RAM_EN);
assign RAM_UPPER = ~(~AS && ~UDS && RAM_EN);


// DUART_EN when addr is > 0xC00000 - 0xD00000
assign DUART = ~(~LDS && ADDR_H[23] && ADDR_H[22] && ~ADDR_H[21] && ~ADDR_H[20]);

endmodule
