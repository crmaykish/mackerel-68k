module dram_controller(
	input RST_n,
	input CLK,
	input CLK_CPU,
	
	input CS_n,
	
	input RW,
	
	input SIZ0, SIZ1,
	
	input AS_n, DS_n,
	
	output DRAM_WR_n,
	
	input [27:0] ADDR,
	
	output [11:0] ADDR_DRAM,
	
	output RAS0_n, RAS1_n, RAS2_n, RAS3_n,
	output CAS0_n, CAS1_n, CAS2_n, CAS3_n,

	output DSACK0_DRAM_n,
	output DSACK1_DRAM_n
);

assign ADDR_DRAM = 12'b0;

assign RAS0_n = 1'b1;
assign RAS1_n = 1'b1;
assign RAS2_n = 1'b1;
assign RAS3_n = 1'b1;

assign CAS0_n = 1'b1;
assign CAS1_n = 1'b1;
assign CAS2_n = 1'b1;
assign CAS3_n = 1'b1;

assign DRAM_WR_n = 1'b1;

assign DSACK0_DRAM_n = 1'b1;
assign DSACK1_DRAM_n = 1'b1;

endmodule
