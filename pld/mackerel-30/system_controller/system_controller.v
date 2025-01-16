module system_controller(
	input RST_n,
	input CLK,

	input [31:28] AH,	// Address bus (high)
	input [19:16] AM,	// Address bus (mid)
	input [3:0] AL,		// Address bus (low)

	output reg DSACK0_n, DSACK1_n,
	output BERR_n,
	output reg AVEC_n = 1'b1,
	output CIIN_n,
	output STERM_n,

	input [2:0] FC,
	output [2:0] IPL_n,
	
	input AS_n, DS_n,
	input SIZ0, SIZ1,
	input RW,
	
	output CS_ROM_n,
	output CS_SRAM_n,
	
	output CS_DRAM_n,
	input DSACK0_DRAM_n,
	input DSACK1_DRAM_n,

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

// Currently unused outputs
assign BERR_n = 1'b1;
assign CIIN_n = 1'b1;
assign STERM_n = 1'b1;

// === BOOT SIGNAL === //

wire BOOT;
boot_signal bs(RST_n, AS_n, BOOT);

// === INTERRUPT HANDLING === //

irq_encoder ie1(
	.irq1(0),
	.irq2(0),
	.irq3(IDE_INT),
	.irq4(0),
	.irq5(~IRQ_DUART_n),
	.irq6(0),
	.irq7(0),
	.ipl0_n(IPL_n[0]),
	.ipl1_n(IPL_n[1]),
	.ipl2_n(IPL_n[2])
);

// Autovector the non-DUART interrupts
always @(posedge CLK) begin
	// Autovector the non-DUART interrupts
	if (~IACK_n && IACK_DUART_n && ~AS_n) AVEC_n <= 1'b0;
	else AVEC_n <= 1'b1;
end

// DUART IACK is mapped to IRQ level 5
assign IACK_DUART_n = ~(~IACK_n && ~AS_n && AL[3:1] == 3'd5);

// === ADDRESS DECODING === //

// ROM at 0xE0000000 (mapped to 0x0000 for the first eight bus cycles)
wire ROM_EN = ~BOOT || (AH == 4'b1110);
assign CS_ROM_n = ~(~CPU_SPACE && ~AS_n && ROM_EN);

// SRAM at 0x00000000
assign CS_SRAM_n = ~(BOOT && ~CPU_SPACE && ~AS_n && ~DS_n && AH == 4'b0000);

// DRAM at 0x80000000
assign CS_DRAM_n = ~(BOOT && ~CPU_SPACE && AH == 4'b1000);

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

// TODO: Does it matter if the requested cycle width is not matched by the DSACK signals?
// e.g. If the CPU requests 8 bits from DRAM, but the DRAM responds with 32

wire IDE_SELECTED = ~AS_n && (~IDE_CS0_n || ~IDE_CS1_n);

// === DSACK GENERATION === //

always @(*) begin
	if (~AVEC_n) begin
		// DSACK and AVEC should not be asserted at the same time
		DSACK0_n <= 1'b1;
		DSACK1_n <= 1'b1;
	end
	else if (~CS_DRAM_n) begin
		DSACK0_n <= DSACK0_DRAM_n;
		DSACK1_n <= DSACK1_DRAM_n;
	end
	else if (IDE_SELECTED) begin
		DSACK0_n <= 1'b1;
		DSACK1_n <= ~IDE_RDY;	// TODO: All IDE access is 16-bit - is that alright?
	end
	else begin
		DSACK0_n <= 1'b0;	// All other accesses are 8-bit
		DSACK1_n <= 1'b1;
	end
end

endmodule
