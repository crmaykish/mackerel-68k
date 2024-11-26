module system_controller(
	input RST_n,
	input CLK,

	input [31:28] AH,	// Address bus (high)
	input [19:16] AM,	// Address bus (mid)
	input [3:0] AL,		// Address bus (low)

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


// === COMMON SIGNALS === //

// Processor is addressing CPU space
wire CPU_SPACE = (FC == 3'b111);
// Processor is responding to an interrupt
wire IACK_n = ~(CPU_SPACE && ~AS_n && AM[19:16] == 4'b1111);

assign BERR_n = 1'b1;
assign AVEC_n = 1'b1;
assign CIIN_n = 1'b1;
assign STERM_n = 1'b1;

// === BOOT SIGNAL === //

wire BOOT;
boot_signal bs(RST_n, AS_n, BOOT);

// === INTERRUPT HANDLING === //

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

// DUART IACK is mapped to IRQ level 5
assign IACK_DUART_n = ~(~IACK_n && ~AS_n && AL[3:1] == 3'd5);

// === ADDRESS DECODING === //

// ROM at 0x80000000 (mapped to 0x0000 for the first eight bus cycles)
wire ROM_EN = ~BOOT || (AH == 4'b1000);
assign CS_ROM_n = ~(~CPU_SPACE && ~AS_n && ROM_EN);

// RAM at 0x00000000
assign CS_SRAM_n = ~(BOOT && ~CPU_SPACE && ~AS_n && ~DS_n && AH == 4'b0000);

// DUART at 0xF0000000
assign CS_DUART_n = ~(BOOT && ~CPU_SPACE && ~AS_n && ~DS_n && AH == 4'b1111 && AM == 4'b0000);

// IDE CS0 at 0xF0010000
assign IDE_CS0_n = ~(BOOT && ~CPU_SPACE && AH == 4'b1111 && AM == 4'b0001);
// IDE CS1 at 0xF0020000
assign IDE_CS1_n = ~(BOOT && ~CPU_SPACE && AH == 4'b1111 && AM == 4'b0010);
// If either IDE CS pin is active, enable the IDE buffers
assign IDE_BUF_n = ~(~IDE_CS0_n || ~IDE_CS1_n);
assign IDE_RD_n = ~(RW && ~AS_n && ~DS_n);
assign IDE_WR_n = ~(~RW && ~AS_n && ~DS_n);

assign CS_FPU_n = 1'b1;

// === DSACK GENERATION === //

assign DSACK0_n = 1'b0;
assign DSACK1_n = 1'b1;

endmodule
