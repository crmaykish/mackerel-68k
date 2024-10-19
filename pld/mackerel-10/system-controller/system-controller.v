module system_controller(
	input CLK,
	input RST,
	
	// TODO: update all the names to include _n if active low

	output CLK_CPU,
	
	output IPL0, IPL1, IPL2,
	
	output BERR, DTACK,
	
	output reg VPA,
	
	input [7:0] DATA,
	
	input [23:14] ADDR_H,
	input [3:1] ADDR_L,
	
	input AS, UDS, LDS, RW,
	
	input FC0, FC1, FC2,
	
	output ROM_LOWER, ROM_UPPER,
	output SRAM_LOWER, SRAM_UPPER,
	
	output EXP,
	input IRQ_EXP,
	input DTACK_EXP,
	output IACK_EXP,
	
	output DUART,
	input IRQ_DUART,
	input DTACK_DUART,
	output IACK_DUART,
	
	output DRAM,
	input DTACK_DRAM,
	
	input IDE_INT,
	output IDE_CS,
	input IDE_RDY,
	output IDE_RD,
	output IDE_WR,
	output IDE_BUF,
	
	output [3:0] GPIO
);

// Unused signals
assign BERR = 1;
assign IACK_EXP = 1'b1;
assign EXP = 1'b1;
assign GPIO[1:0] = 2'b0;

// Reconstruct the full address bus
wire [24:0] ADDR_FULL = {ADDR_H, 10'b0, ADDR_L, 1'b0};

// CPU is responding to an interrupt request
wire IACK = ~(FC0 && FC1 && FC2);

assign IACK_DUART = ~(~IACK && ~AS && ADDR_L[3:1] == 3'd5);

// DTACK from DUART
wire DTACK0 = ((~DUART || ~IACK_DUART) && DTACK_DUART);
// DTACK from DRAM
wire DTACK1 = (~DRAM && DTACK_DRAM);
// DTACK to CPU
assign DTACK = DTACK0 || DTACK1 || ~VPA;	// NOTE: DTACK and VPA cannot be LOW at the same time


// BOOT signal generation
wire BOOT;
boot_signal bs1(RST, AS, BOOT);

// Generate CPU clock from source oscillator
clock_gen cg1(CLK, CLK_CPU);

// Encode interrupt sources to the CPU's IPL pins
irq_encoder ie1(
	.irq1(0),
	.irq2(0),
	.irq3(0),
	.irq4(0),
	.irq5(~IRQ_DUART),
	.irq6(IRQ_TIMER),
	.irq7(0),
	.ipl0_n(IPL0),
	.ipl1_n(IPL1),
	.ipl2_n(IPL2)
);

// Generate a periodic timer interrupt (100 Hz)
reg IRQ_TIMER = 0;

reg[17:0] timer_buf = 0;
always @(posedge CLK_CPU) begin
	timer_buf <= timer_buf + 1'b1;
	
	if (timer_buf == 18'd100000) begin
		IRQ_TIMER <= 1;
		timer_buf <= 18'b0;
	end
	
	// TODO: does VPA go LOW as soon as the IRQ goes low?
	
	// autovector the non-DUART interrupts
	if (~IACK && IACK_DUART && ~AS) begin
		VPA <= 1'b0;
		IRQ_TIMER <= 0;
	end
	else VPA <= 1'b1;
end

//================================//
// Address Decoding
//================================//

// ROM at 0xF00000 (0x000000 on BOOT)
wire ROM_EN = ~BOOT || (IACK && ADDR_FULL >= 24'hF00000 && ADDR_FULL < 24'hFF4000);
assign ROM_LOWER = ~(~AS && ~LDS && ROM_EN);
assign ROM_UPPER = ~(~AS && ~UDS && ROM_EN);

// SRAM enabled at 0x000000 - 0x100000 (except at boot)
/*
wire RAM_EN = BOOT && IACK && ADDR_FULL < 24'h100000;
assign SRAM_LOWER = ~(~AS && ~LDS && RAM_EN);
assign SRAM_UPPER = ~(~AS && ~UDS && RAM_EN);
*/

// DUART at 0xFF8000
assign DUART = ~(BOOT && IACK && ~LDS && ADDR_FULL >= 24'hFF8000 && ADDR_FULL < 24'hFFC000);

// IDE at 0xFF4000 and 0xFFC000
assign IDE_CS = ~(BOOT && IACK && ADDR_FULL >= 24'hFFC000);
assign GPIO[2] = ~(BOOT && IACK && ADDR_FULL >= 24'hFF4000 && ADDR_FULL < 24'hFF8000);	// IDE CS1 pin (bodge)
assign IDE_BUF = ~(~IDE_CS || ~GPIO[2]);
assign IDE_RD = ~(RW && ~AS && ~UDS);
assign IDE_WR = ~(~RW && ~AS && ~UDS);
assign GPIO[3] = ~RW;	// IDE buffer DIR pin (bodge)

// DRAM at 0x000000 - 0xF00000
wire DRAM_EN = BOOT && IACK && ADDR_FULL < 24'hF00000;
assign DRAM = ~DRAM_EN;

endmodule
