module system_controller(
	input CLK,
	input RST_n,

	output CLK_CPU,
	
	output IPL0_n, IPL1_n, IPL2_n,
	
	output DTACK_n,
	
	output reg VPA_n,
	
	inout [7:0] DATA,

	input [23:14] ADDR_H,
	input [3:1] ADDR_L,

	input AS_n, UDS_n, LDS_n, RW,

	input FC0, FC1, FC2,

	output CS_ROM0_n, CS_ROM1_n,
	output CS_SRAM0_n, CS_SRAM1_n,

	output CS_EXP_n,
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

	// SPI master + W5500
	output SPI_SCK,
	output SPI_MOSI,
	input SPI_MISO,
	output NIC_CS_n,
	input NIC_INT_n
);

// Unused signals
assign IACK_EXP_n = 1;
assign CS_EXP_n = 1'b1;

// CPU is responding to an interrupt request
wire IACK_n = ~(FC0 && FC1 && FC2);

assign IACK_DUART_n = ~(~IACK_n && ~AS_n && ADDR_L[3:1] == 3'd5);

// DTACK from DUART
//wire DTACK0 = ((~CS_DUART_n || ~IACK_DUART_n) && DTACK_DUART_n);
wire DTACK0 = 1'b0;	// DUART DTACK is always low anyway?
// DTACK from DRAM
wire DTACK1 = (~CS_DRAM_n && DTACK_DRAM_n);
// DTACK from IDE
//wire DTACK2 = ((~CS_IDE0_n || ~CS_IDE1_n) && ~IDE_RDY);
wire DTACK2 = 1'b0;
// DTACK from SPI
wire CS_SPI_n;
wire spi_dtack_n;
wire DTACK3 = (~CS_SPI_n && spi_dtack_n);
// DTACK to CPU
assign DTACK_n = DTACK0 || DTACK1 || DTACK2 || DTACK3 || ~VPA_n;	// NOTE: DTACK and VPA cannot be LOW at the same time

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
	.irq4(~NIC_INT_n),
	.irq5(~IRQ_DUART_n),
	.irq6(0),
	.irq7(0),
	.ipl0_n(IPL0_n),
	.ipl1_n(IPL1_n),
	.ipl2_n(IPL2_n)
);


always @(posedge CLK_CPU) begin
	// Autovector the non-DUART interrupts
	if (~IACK_n && IACK_DUART_n && ~AS_n) VPA_n <= 1'b0;
	else VPA_n <= 1'b1;
end

//================================//
// Address Decoding
//================================//

//   0x000000-0xEFFFFF  DRAM     (~HIGH1M)
//   0xF00000-0xFEFFFF  ROM      (HIGH1M & ~TOP64K)
//   0xFF0000-0xFF3FFF  SPI
//   0xFF4000-0xFF7FFF  IDE1
//   0xFF8000-0xFFBFFF  DUART
//   0xFFC000-0xFFFFFF  IDE0
wire HIGH1M = (ADDR_H[23:20] == 4'hF);
wire TOP64K = HIGH1M && (ADDR_H[19:16] == 4'hF);
wire [1:0] sub = ADDR_H[15:14];
wire DECODE = BOOT && IACK_n;

// ROM at 0xF00000 - 0xFEFFFF (shadowed at 0x000000 during BOOT)
wire ROM_EN = ~BOOT || (IACK_n && HIGH1M && ~TOP64K);
assign CS_ROM0_n = ~(~AS_n && ~LDS_n && ROM_EN);
assign CS_ROM1_n = ~(~AS_n && ~UDS_n && ROM_EN);

//================================//
// SPI master + W5500 NIC
//================================//

// SPI registers at 0xFF0000 - 0xFF3FFF, low byte lane (LDS / D0-7).
assign CS_SPI_n = ~(DECODE && ~LDS_n && TOP64K && sub == 2'b00);

wire [7:0] spi_data_out;
wire spi_irq;

spi spi1(
	.clk(CLK),			// run the SPI in the 20 MHz oscillator domain
	.rst_n(RST_n),
	.cs_n(CS_SPI_n),
	.reg_addr(ADDR_L[3:1]),
	.rwn(RW),
	.ds_n(LDS_n),
	.data_in(DATA),
	.data_out(spi_data_out),
	.dtack_n(spi_dtack_n),
	.irq(spi_irq),
	.mosi(SPI_MOSI),
	.sck(SPI_SCK),
	.miso(SPI_MISO),
	.nic_cs_n(NIC_CS_n)
);

// Drive the low data byte back to the CPU only on an SPI read
assign DATA = (~CS_SPI_n && RW && ~LDS_n) ? spi_data_out : 8'bz;

// SRAM disabled (replaced by DRAM)
assign CS_SRAM0_n = 1'b1;
assign CS_SRAM1_n = 1'b1;

// DRAM at 0x000000 - 0xEFFFFF
assign CS_DRAM_n = ~(DECODE && ~HIGH1M);

// DUART at 0xFF8000 - 0xFFBFFF (low byte lane)
assign CS_DUART_n = ~(DECODE && ~LDS_n && TOP64K && sub == 2'b10);

// IDE at 0xFF4000 (IDE1) and 0xFFC000 (IDE0)
assign CS_IDE0_n = ~(DECODE && TOP64K && sub == 2'b11);
assign CS_IDE1_n = ~(DECODE && TOP64K && sub == 2'b01);
assign IDE_BUF_n = ~(~CS_IDE0_n || ~CS_IDE1_n);
assign IDE_RD_n = ~(RW && ~AS_n && ~UDS_n);
assign IDE_WR_n = ~(~RW && ~AS_n && ~UDS_n);

endmodule
