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
	
	input FC0, FC1, FC2,
	
	output ROM_LOWER, ROM_UPPER,
	output RAM_LOWER, RAM_UPPER,
	output DUART,
	output EXP,
	
	output IACK_DUART
	
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

reg clk_buf = 0;

always @(posedge CLK) clk_buf <= clk_buf + 1;

assign CLK_CPU = clk_buf;



always @(posedge CLK_CPU) begin
	if (~RST) LED <= 0;
	// LED at 0xF00000
	if (ADDR_H[23] && ADDR_H[22] && ADDR_H[21] && ADDR_H[20]) begin
		if (~AS) LED <= DATA[2:0];
	end
end

// ROM_EN at 0x000000
wire ROM_EN = ~ADDR_H[23] && ~ADDR_H[22] && ~ADDR_H[21] && ~ADDR_H[20];

assign ROM_LOWER = ~(~AS && ~LDS && ROM_EN);
assign ROM_UPPER = ~(~AS && ~UDS && ROM_EN);

// RAM_EN at 0x800000
wire RAM_EN = ADDR_H[23] && ~ADDR_H[22];

assign RAM_LOWER = ~(~AS && ~LDS && RAM_EN);
assign RAM_UPPER = ~(~AS && ~UDS && RAM_EN);


// DUART_EN when addr is > 0xC00000
assign DUART = ~(~AS && ~LDS && ADDR_H[23] && ADDR_H[22]);

//assign DUART = 1;

endmodule
