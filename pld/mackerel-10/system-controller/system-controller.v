module system_controller(
	input CLK,
	input RST_n,

	output CLK_CPU,
	
	output IPL0_n, IPL1_n, IPL2_n,
	
	output DTACK_n,
	
	output reg VPA_n,
	
	input [7:0] DATA,
	
	input [23:14] ADDR_H,
	input [3:1] ADDR_L,
	
	input AS_n, UDS_n, LDS_n, RW,
	
	input FC0, FC1, FC2,
	
	output CS_ROM0_n, CS_ROM1_n,
	output CS_SRAM0_n, CS_SRAM1_n,
	
	output CS_EXP_n,
	input IRQ_EXP_n,
	input DTACK_EXP_n,
	output IACK_EXP_n,
	
	output CS_DUART_n,
	input IRQ_DUART_n,
	input DTACK_DUART_n,
	output IACK_DUART_n,
	
	output CS_DRAM_n,
	input DTACK_DRAM_n,
	
	input IDE_INT,
	output CS_IDE0_n,
	output CS_IDE1_n,
	input IDE_RDY,
	output IDE_RD_n,
	output IDE_WR_n,
	output IDE_BUF_n,
	
	output [3:0] GPIO
);

// Source oscillator frequency
localparam OSC_FREQ_HZ = 24000000;
// CPU frequency (half the oscillator frequency)
localparam CPU_FREQ_HZ = OSC_FREQ_HZ / 2;
// Frequency of the periodic timer interrupt
localparam TIMER_FREQ_HZ = 50;
// CPU cycles between timer interrupts
localparam TIMER_DELAY_CYCLES = CPU_FREQ_HZ / TIMER_FREQ_HZ;

// Unused signals
assign IACK_EXP_n = 1;
assign CS_EXP_n = 1'b1;

assign GPIO = 4'b0;

// Reconstruct the full address bus
wire [24:0] ADDR_FULL = {ADDR_H, 10'b0, ADDR_L, 1'b0};

// CPU is responding to an interrupt request
wire IACK_n = ~(FC0 && FC1 && FC2);

assign IACK_DUART_n = ~(~IACK_n && ~AS_n && ADDR_L[3:1] == 3'd5);

// DTACK from DUART
wire DTACK0 = ((~CS_DUART_n || ~IACK_DUART_n) && DTACK_DUART_n);
// DTACK from DRAM
wire DTACK1 = (~CS_DRAM_n && DTACK_DRAM_n);
// DTACK from IDE
//wire DTACK2 = ((~CS_IDE0_n || ~CS_IDE1_n) && ~IDE_RDY);
wire DTACK2 = 1'b0;
// DTACK to CPU
assign DTACK_n = DTACK0 || DTACK1 || DTACK2 || ~VPA_n;	// NOTE: DTACK and VPA cannot be LOW at the same time

// BOOT signal generation
wire BOOT;
boot_signal bs1(RST_n, AS_n, BOOT);

// Generate CPU clock from source oscillator
clock_gen cg1(CLK, CLK_CPU);

// Encode interrupt sources to the CPU's IPL pins
irq_encoder ie1(
	.irq1(0),
	.irq2(0),
	.irq3(IDE_INT),
	.irq4(0),
	.irq5(~IRQ_DUART_n),
	.irq6(IRQ_TIMER),
	.irq7(0),
	.ipl0_n(IPL0_n),
	.ipl1_n(IPL1_n),
	.ipl2_n(IPL2_n)
);

reg[23:0] clock_cycles = 0;
reg IRQ_TIMER = 0;

always @(posedge CLK_CPU) begin
	clock_cycles <= clock_cycles + 1'b1;
	
	// Generate a periodic interrupt timer
	if (RST_n && clock_cycles == TIMER_DELAY_CYCLES) begin
		IRQ_TIMER <= 1;
		clock_cycles <= 24'b0;
	end
	
	// Autovector the non-DUART interrupts
	if (~IACK_n && IACK_DUART_n && ~AS_n) begin
		VPA_n <= 1'b0;
		IRQ_TIMER <= 0;
	end
	else VPA_n <= 1'b1;
end

//================================//
// Address Decoding
//================================//

// ROM at 0xF00000 - 0xFF4000 (0x000000 on BOOT)
wire ROM_EN = ~BOOT || (IACK_n && ADDR_FULL >= 24'hF00000 && ADDR_FULL < 24'hFF4000);
assign CS_ROM0_n = ~(~AS_n && ~LDS_n && ROM_EN);
assign CS_ROM1_n = ~(~AS_n && ~UDS_n && ROM_EN);

// SRAM enabled at 0xE00000 - 0xF00000 (1 MB)
wire RAM_EN = BOOT && IACK_n && ADDR_FULL >= 24'hE00000 && ADDR_FULL < 24'hF00000;
assign CS_SRAM0_n = ~(~AS_n && ~LDS_n && RAM_EN);
assign CS_SRAM1_n = ~(~AS_n && ~UDS_n && RAM_EN);

// DRAM at 0x000000 - 0xE00000 (14 MB)
assign CS_DRAM_n = ~(BOOT && IACK_n && ADDR_FULL < 24'hE00000);

// DUART at 0xFF8000
assign CS_DUART_n = ~(BOOT && IACK_n && ~LDS_n && ADDR_FULL >= 24'hFF8000 && ADDR_FULL < 24'hFFC000);

// IDE at 0xFF4000 and 0xFFC000
assign CS_IDE0_n = ~(BOOT && IACK_n && ADDR_FULL >= 24'hFFC000);
assign CS_IDE1_n = ~(BOOT && IACK_n && ADDR_FULL >= 24'hFF4000 && ADDR_FULL < 24'hFF8000);
assign IDE_BUF_n = ~(~CS_IDE0_n || ~CS_IDE1_n);
assign IDE_RD_n = ~(RW && ~AS_n && ~UDS_n);
assign IDE_WR_n = ~(~RW && ~AS_n && ~UDS_n);

endmodule
