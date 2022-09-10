module mack_decoder_v2(
	input CLK,
	input RST,
	input [23:15] ADDR,
	input AS,
	input DTACK_IN,
    input IACK,
	output ROMEN,
	output RAMEN,        
	output MFPEN,
	output DTACK
);

	// Generate the BOOT signal for the first 8 memory accesses after reset
	reg BOOT = 1'b0;
	reg [3:0] bus_cycles = 0;
	reg got_cycle = 1'b0;
	
	always @(posedge CLK) begin
		if (~RST) begin 
			bus_cycles = 0;
			BOOT <= 1'b0;
		end
		else begin
			if (~BOOT) begin
				if (~AS) begin
					if(~got_cycle) begin
						bus_cycles <= bus_cycles + 4'b1;
						got_cycle <= 1'b1;
					end
				end
				else begin 
					got_cycle <= 1'b0;
					if (bus_cycles > 4'd8) BOOT <= 1'b1;
				end
			end
		end
	end
	
	// Define memory map
	
	// 0x380000 - 256K
	assign ROMEN = ~(IACK & ~AS & (~BOOT | (~ADDR[23] & ~ADDR[22] & ADDR[21] & ADDR[20] & ADDR[19] & ~ADDR[18])));
	
	// 0x3C0000 - 256K
	assign MFPEN = ~(IACK & ~AS & BOOT & ~ADDR[23] & ~ADDR[22] & ADDR[21] & ADDR[20] & ADDR[19] & ADDR[18]);
	
	// TODO: use the GAL on the RAM board to determine individual CS pins based on address

	assign RAMEN = ~(IACK & ~AS & BOOT & ~ADDR[23] & ~ADDR[22] & ~ADDR[21] & ~ADDR[20] & ~ADDR[19]);
	
	// assign RAMEN = ~(IACK & ~AS & BOOT);

	// Generate DTACK signal
	assign DTACK = (MFPEN & DTACK_IN & ~IACK) | (~MFPEN & DTACK_IN & IACK);
	
endmodule
