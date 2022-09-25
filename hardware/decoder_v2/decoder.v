module decoder(
	input CLK,
	input RST,
	input AS,
	input DTACK_IN,
	input IACK,
	input [23:16] ADDR,
	output ROMEN,
	output RAMEN,
	output DUARTEN,
	output DTACK
);

	// ======================
	// BOOT Signal
	// ======================

	reg BOOT = 1'b0;
	reg [3:0] bus_cycles = 0;
	
	always @(posedge AS) begin
		if (~RST) begin 
			bus_cycles = 0;
			BOOT <= 1'b0;
		end
		else begin
			if (~BOOT) begin
				bus_cycles <= bus_cycles + 4'b1;
				if (bus_cycles == 4'd8) BOOT <= 1'b1;
			end
		end
	end

	// ======================
	// Address Decoding
	// ======================

	assign RAMEN = ~(IACK & ~AS & BOOT);

	// 0x380000 - 256K
	assign ROMEN = ~(IACK & ~AS & (~BOOT | (~ADDR[23] & ~ADDR[22] & ADDR[21] & ADDR[20] & ADDR[19] & ~ADDR[18])));

	// 0x3E0000
	assign DUARTEN = ~(IACK & ~AS & BOOT & ~ADDR[23] & ~ADDR[22] & ADDR[21] & ADDR[20] & ADDR[19] & ADDR[18] & ADDR[17]);

	// DTACK
	// TODO: Implement proper DTACK handling for DUART and interrupt handling
	assign DTACK = 0;

	// I don't think this is right
	// assign DTACK = ~((~DUARTEN * ~DTACK_IN * IACK) | DUARTEN);


endmodule
