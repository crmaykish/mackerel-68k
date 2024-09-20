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
	
	input RW,
	
	input FC0, FC1, FC2,
	
	output ROM_LOWER, ROM_UPPER,
	output RAM_LOWER, RAM_UPPER,
	
	output EXP,
	input DTACK_EXP,
	
	output DUART,
	input IRQ_DUART,
	input DTACK_DUART,
	output IACK_DUART,
	
	output [7:0] GPIO
);

assign GPIO[2] = ~RW;	//R
assign GPIO[3] = RW;		//W

assign GPIO[0] = ~DRAM_EN;	// CSO
assign GPIO[1] = 1'b1;	// CS1

// Reconstruct the full address bus
wire [24:0] ADDR_FULL = {ADDR_H, 9'b0, ADDR_L, 1'b0};

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
			bus_cycles <= bus_cycles + 4'b1;
			if (bus_cycles == 4'd4) BOOT <= 1'b1;
		end
	end
end

// Generate CPU clock from source oscillator
reg [2:0] clk_buf = 0;
assign CLK_CPU = clk_buf[0];	// Divide source clock by 2 to get CPU clock
always @(posedge CLK) clk_buf <= clk_buf + 3'b1;

// Handle memory addressable GPIO on CPLD
always @(posedge CLK_CPU) begin
	if (~RST)
		begin
			LED <= 0;
			//GPIO <= 0;
		end
	else
		// LED at 0xF00001
		if (ADDR_H[23] && ADDR_FULL == 24'hF00000) begin
			if (~LDS && ~RW) LED <= DATA[2:0];
		end
/*
		// GPIO at 0xF00003
		if (ADDR_H[23] && ADDR_FULL == 24'hF00002) begin
			if (~LDS && ~RW) GPIO <= DATA[7:0];
		end
*/
end

// ROM enabled at 0xE00000 - 0xF00000
wire ROM_EN = ~BOOT || (IACK && ADDR_FULL >= 24'hE00000 && ADDR_FULL < 24'hF00000);
assign ROM_LOWER = ~(~AS && ~LDS && ROM_EN);
assign ROM_UPPER = ~(~AS && ~UDS && ROM_EN);

// SRAM enabled at 0x000000 - 0x100000 (except at boot)
wire RAM_EN = BOOT && IACK && ADDR_FULL < 24'h100000;
assign RAM_LOWER = ~(~AS && ~LDS && RAM_EN);
assign RAM_UPPER = ~(~AS && ~UDS && RAM_EN);

// DUART_EN when addr is > 0xC00000 - 0xD00000
assign DUART = ~(BOOT && IACK && ~LDS && ADDR_FULL >= 24'hC00000 && ADDR_FULL < 24'hD00000);

// DRAM at 0x100000 - 0x900000
wire DRAM_EN = BOOT && IACK && ADDR_FULL >= 24'h100000 && ADDR_FULL < 24'h900000;
assign EXP = ~DRAM_EN;

endmodule
