module decoder(
	input CLK,
	input RST,
	input AS,
	input DTACK_IN,
	input IACK,
	input [23:16] ADDR,
	output ROMEN,
	output RAMEN,
	output MFPEN,
	output DUARTEN,
	output DTACK,
	output LED_BLUE
);

	// ======================
	// Counter
	// ======================

	// reg [20:0] counter = 0;

	// always @(posedge CLK) begin
	// 	counter <= counter + 1;
	// end

	// assign LED_BLUE = counter[20];

	assign LED_BLUE = 1;	// LED signal is inverted?

	// ======================
	// BOOT Signal
	// ======================

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

	// ======================
	// Address Decoding
	// ======================

	// 0x380000 - 256K
	assign ROMEN = ~(IACK & ~AS & (~BOOT | (~ADDR[23] & ~ADDR[22] & ADDR[21] & ADDR[20] & ADDR[19] & ~ADDR[18])));
	
	assign RAMEN = ~(IACK & ~AS & BOOT);

	// 0x3C0000
	assign MFPEN = ~(~ADDR[23] & ~ADDR[22] & ADDR[21] & ADDR[20] & ADDR[19] & ADDR[18] & ~ADDR[17]);
	
	// 0x3E0000
	assign DUARTEN = ~(IACK & ~AS & BOOT & ~ADDR[23] & ~ADDR[22] & ADDR[21] & ADDR[20] & ADDR[19] & ADDR[18] & ADDR[17]);

	// DTACK
	// TODO: ignoring DUART DTACK
	assign DTACK = (MFPEN & DTACK_IN & ~IACK) | (~MFPEN & DTACK_IN & IACK);


endmodule
