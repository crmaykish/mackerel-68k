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
	
	output CS_FPU_n,
	input DSACK0_FPU_n,
	input DSACK1_FPU_n,
	
	output CS_DUART_n,
	output IACK_DUART_n,
	input IRQ_DUART_n,
	input DTACK_DUART_n,
	
	output IDE_BUF_n,
	output IDE_CS0_n,
	output IDE_CS1_n,
	output IDE_WR_n,
	output IDE_RD_n,
	input IDE_RDY,
	input IDE_INT,
	
	output P5, P6, P8, P9, P10
);

// Reconstruct the full address bus value from components
//wire [31:0] ADDR = {AH, 8'b0, AM, 12'b0, AL};

// TODO: might have to break up IACK and CPU space for coprocessor logic
wire IACK_n = ~(FC == 3'b111 && AM[19:16] == 4'b1111);

assign DSACK0_n = 1'b0;	// All memory cycles are 8-bit
assign DSACK1_n = 1'b1;
assign BERR_n = 1'b1;
assign AVEC_n = 1'b1;
assign CIIN_n = 1'b1;
assign STERM_n = 1'b1;

assign CS_FPU_n = 1'b1;

assign IDE_BUF_n = 1'b1;


assign P5 = AS_n;
assign P6 = IACK_DUART_n;
assign P8 = IRQ_DUART_n;

wire BOOT;
boot_signal bs(RST_n, AS_n, BOOT);

irq_encoder ie1(
	.irq1(0),
	.irq2(0),
	.irq3(0),
	.irq4(0),
	.irq5(~IRQ_DUART_n),
	.irq6(0),
	.irq7(0),
	.ipl0_n(IPL_n[0]),
	.ipl1_n(IPL_n[1]),
	.ipl2_n(IPL_n[2])
);

assign IACK_DUART_n = ~(~IACK_n && ~AS_n && AL[3:1] == 3'd5);

// ROM at 0x0000 for first two memory cycles, 0x80000000 after that
wire ROM_EN = ~BOOT || (IACK_n && AH[31] && ~AH[30]);
assign CS_ROM_n = ~(~AS_n && ~DS_n && ROM_EN);

// RAM at 0x0000 after first two memory cycles
assign CS_SRAM_n = ~(IACK_n && BOOT && ~AS_n && ~DS_n && ~AH[31] && ~AH[30]);

// DUART at 0xC0000000
assign CS_DUART_n = ~(IACK_n && BOOT && ~AS_n && ~DS_n && AH[31] && AH[30]);

endmodule
