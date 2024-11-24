module system_controller(
	input RST_n,
	input CLK,
	
	// Address bus components
	input [3:0] AL,
	input [19:16] AM,
	input [31:28] AH,
	
	output DSACK0_n, DSACK1_n,
	output BERR_n,
	output AVEC_n,
	output CIIN_n,
	output STERM_n,
	
	input [2:0] FC,
	output [2:0] IPL_n,
	
	input AS_n, DS_n,
	input SIZ0, SIZ1,
	input RW,
	
	output CS_ROM_n,
	output CS_SRAM_n,
	output CS_DUART_n,
	output IACK_DUART_n,
	
	output P5, P6, P8, P9, P10
);

// Reconstruct the full address bus value from components
//wire [31:0] ADDR = {AH, 8'b0, AM, 12'b0, AL};

assign DSACK0_n = 1'b0;	// All memory cycles are 8-bit
assign DSACK1_n = 1'b1;
assign BERR_n = 1'b1;
assign AVEC_n = 1'b1;
assign CIIN_n = 1'b1;
assign STERM_n = 1'b1;

wire BOOT;
boot_signal bs(RST_n, AS_n, BOOT);

assign IPL_n = 3'b111;

assign IACK_DUART_n = 1'b1;

// ROM at 0x0000 for first two memory cycles, 0x80000000 after that
wire ROM_EN = ~BOOT || (AH[31] && ~AH[30]);
assign CS_ROM_n = ~(~AS_n && ~DS_n && ROM_EN);

// RAM at 0x0000 after first two memory cycles
assign CS_SRAM_n = ~(BOOT && ~AS_n && ~DS_n && ~AH[31] && ~AH[30]);

// DUART at 0xC0000000
assign CS_DUART_n = ~(BOOT && ~AS_n && ~DS_n && AH[31] && AH[30]);

endmodule
