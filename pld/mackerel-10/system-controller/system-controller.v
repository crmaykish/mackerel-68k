module system_controller(
	input CLK,
	input RST,
	
	output CLK_CPU,
	
	output IPL0, IPL1, IPL2,
	
	output BERR, DTACK, VPA,
	
	input [7:0] DATA,
	
	input [23:14] ADDR_H,
	input [3:1] ADDR_L,
	
	input AS, UDS, LDS,
	
	input RW,
	
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
	
	output reg [3:0] GPIO
);

//assign GPIO[2] = ~(RW && ~AS && ~LDS);	//R
//assign GPIO[3] = ~(~RW && ~AS && ~LDS);		//W

//assign GPIO[0] = ~(BOOT && ADDR_FULL >= 24'hA00000 && ADDR_FULL < 24'hA00F00);	// CSO
//assign GPIO[1] = 1'b1;	// CS1

//assign GPIO[4] = IDE_RDY; input

// Reconstruct the full address bus
wire [24:0] ADDR_FULL = {ADDR_H, 10'b0, ADDR_L, 1'b0};

assign BERR = 1;
assign VPA = 1;

assign IPL0 = IRQ_DUART;
assign IPL1 = 1;
assign IPL2 = 1;

wire IACK = ~(FC0 && FC1 && FC2);

assign IACK_DUART = ~(~IACK && ~AS && ~ADDR_L[3] && ~ADDR_L[2] && ADDR_L[1]);

// TODO: confirm this DTACK logic
//wire DTACK0 = ~DTACK_DUART && (~DUART || ~IACK_DUART);
//wire DTACK1 = ~DTACK_EXP && ~EXP;
//assign DTACK = ~(DTACK0 || DTACK1 || (DUART && EXP));

//assign DTACK = (~EXP && DTACK_EXP);

assign DTACK = 1'b0;

assign IACK_EXP = 1'b1;

assign DRAM = 1'b1;

assign EXP = 1'b1;


// Generate BOOT signal for first four bus cycles after reset
reg BOOT = 1'b0;
reg [2:0] bus_cycles = 0;

always @(posedge AS) begin
	if (~RST) begin 
		bus_cycles = 0;
		BOOT <= 1'b0;
	end
	else begin
		if (~BOOT) begin
			bus_cycles <= bus_cycles + 3'b1;
			if (bus_cycles == 4'd4) BOOT <= 1'b1;
		end
	end
end

// Generate CPU clock from source oscillator
reg [2:0] clk_buf = 0;
assign CLK_CPU = clk_buf[2];	// Divide 40MHz source clock by 8 to get CPU clock
always @(posedge CLK) clk_buf <= clk_buf + 1'b1;

// Handle memory addressable GPIO on CPLD
/*
always @(posedge CLK_CPU) begin
	if (~RST)
		begin
			GPIO <= 0;
		end
	else
		// GPIO at 0xF00003
		if (ADDR_FULL == 24'hF00002) begin
			if (~LDS && ~RW) GPIO <= DATA[3:0];
		end

end
*/

// ROM enabled at 0xE00000 - 0xF00000
wire ROM_EN = ~BOOT || (IACK && ADDR_FULL >= 24'hE00000 && ADDR_FULL < 24'hF00000);
assign ROM_LOWER = ~(~AS && ~LDS && ROM_EN);
assign ROM_UPPER = ~(~AS && ~UDS && ROM_EN);

// SRAM enabled at 0x000000 - 0x100000 (except at boot)
wire RAM_EN = BOOT && IACK && ADDR_FULL < 24'h100000;
assign SRAM_LOWER = ~(~AS && ~LDS && RAM_EN);
assign SRAM_UPPER = ~(~AS && ~UDS && RAM_EN);

// DUART_EN when addr is > 0xC00000 - 0xD00000
assign DUART = ~(BOOT && IACK && ~LDS && ADDR_FULL >= 24'hC00000 && ADDR_FULL < 24'hD00000);

// IDE at 0xA00000
assign IDE_CS = ~(BOOT && ADDR_FULL >= 24'hA00000 && ADDR_FULL < 24'hA00F00); ///LDS?
assign IDE_BUF = ~(BOOT && ADDR_FULL >= 24'hA00000 && ADDR_FULL < 24'hA00F00);
assign IDE_RD = ~(RW && ~AS && ~LDS);
assign IDE_WR = ~(~RW && ~AS && ~LDS);

// DRAM at 0x100000 - 0x900000
//wire DRAM_EN = BOOT && IACK && ADDR_FULL >= 24'h100000 && ADDR_FULL < 24'h900000;
//assign EXP = ~DRAM_EN;

endmodule
