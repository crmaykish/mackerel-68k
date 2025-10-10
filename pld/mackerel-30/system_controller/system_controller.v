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

// DUART IACK is mapped to IRQ level 5
assign IACK_DUART_n = ~(~IACK_n && AL[3:1] == 3'd5);

// === ADDRESS DECODING === //

// ROM at 0xE0000000 (mapped to 0x0000 for the first eight bus cycles)
wire ROM_EN = ~BOOT || (AH == 4'b1110);
assign CS_ROM_n = ~(~CPU_SPACE && ~AS_n && ROM_EN);

// SRAM is currently disabled
assign CS_SRAM_n = 1'b1;

// DRAM at 0x0000_0000 - 0x7FFF_FFFF
assign CS_DRAM_n = ~(BOOT && ~CPU_SPACE && AH == 4'b0000);

// DUART at 0xF0000000
assign CS_DUART_n = ~(BOOT && ~CPU_SPACE && ~AS_n && ~DS_n && AH == 4'b1111 && AM == 4'b0000);

// IDE CS0 at 0xF0010000
assign IDE_CS0_n = ~(BOOT && ~CPU_SPACE && AH == 4'b1111 && AM == 4'b0001);
// IDE CS1 at 0xF0020000
assign IDE_CS1_n = ~(BOOT && ~CPU_SPACE && AH == 4'b1111 && AM == 4'b0010);

// IDE is selected if either CS0 or CS1 is active
wire CS_IDE_n = ~(~AS_n && (~IDE_CS0_n || ~IDE_CS1_n));

assign IDE_BUF_n = CS_IDE_n;
assign IDE_RD_n = ~(RW && ~AS_n && ~DS_n);
assign IDE_WR_n = ~(~RW && ~AS_n && ~DS_n);

// TODO: FPU support is currently disabled
assign CS_FPU_n = 1'b1;

// === WAIT STATE GENERATION === //

parameter IDE_WAIT = 0;    // IDE wait states (cycles)
parameter DUART_WAIT = 0;  // DUART wait states (cycles)

reg [3:0] ide_wait_cnt = 0;
reg [3:0] duart_wait_cnt = 0;
reg ide_waiting = 0;
reg duart_waiting = 0;

// Clocked wait-state generator
always @(posedge CLK or negedge RST_n) begin
    if (!RST_n) begin
        ide_wait_cnt <= 0;
		ide_waiting <= 0;
        duart_wait_cnt <= 0;
        duart_waiting <= 0;
    end
	else begin
        // IDE wait state logic
        if (~CS_IDE_n && ~ide_waiting) begin
            ide_wait_cnt <= 0;
            ide_waiting <= 1;
        end else if (ide_waiting && ide_wait_cnt < IDE_WAIT) begin
            ide_wait_cnt <= ide_wait_cnt + 1;
        end else if (ide_waiting && ide_wait_cnt >= IDE_WAIT) begin
            ide_waiting <= 0;
        end

        // DUART wait state logic
        if (~CS_DUART_n && ~duart_waiting) begin
            duart_wait_cnt <= 0;
            duart_waiting <= 1;
        end else if (duart_waiting && duart_wait_cnt < DUART_WAIT) begin
            duart_wait_cnt <= duart_wait_cnt + 1;
        end else if (duart_waiting && duart_wait_cnt >= DUART_WAIT) begin
            duart_waiting <= 0;
        end
    end
end

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
always @(*) begin
    if (~IACK_n && IACK_DUART_n && ~AS_n)
        AVEC_n = 1'b0;
    else
        AVEC_n = 1'b1;
end

// === DSACK GENERATION === //
always @(*) begin
    // Handle interrupt acknowledge cycles
    if (~IACK_n) begin
        if (~IACK_DUART_n) begin
            DSACK0_n <= 1'b0;
            DSACK1_n <= 1'b1;
        end
    end
    else if (~CS_DRAM_n) begin
        DSACK0_n <= DSACK0_DRAM_n;
        DSACK1_n <= DSACK1_DRAM_n;
    end
    else if (~CS_IDE_n) begin
        // Insert IDE wait states before asserting DSACK
        if (ide_waiting) begin
            DSACK0_n <= 1'b1;
            DSACK1_n <= 1'b1;
		end
        else begin
            DSACK0_n <= 1'b1;
            DSACK1_n <= 1'b0;
        end
    end
    else if (~CS_DUART_n) begin
        // Insert DUART wait states before asserting DSACK
        if (duart_waiting) begin
            DSACK0_n <= 1'b1;
            DSACK1_n <= 1'b1;
		end
        else begin
            DSACK0_n <= 1'b0;
            DSACK1_n <= 1'b1;
        end
    end
    else if (~CS_ROM_n || ~CS_SRAM_n) begin
        // ROM and SRAM respond immediately
        DSACK0_n <= 1'b0;
        DSACK1_n <= 1'b1;
    end
    else begin
        DSACK0_n <= 1'b1;
        DSACK1_n <= 1'b1;
    end
end

endmodule
