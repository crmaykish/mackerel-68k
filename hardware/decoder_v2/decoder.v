module decoder(
	input CLK,
	input RST,
	input AS,
	// output VALID_ADDRESS,
	output LED_BLUE
);

	// ======================
	// Counter
	// ======================

	reg [20:0] counter = 0;

	always @(posedge CLK) begin
		counter <= counter + 1;
	end

	assign LED_BLUE = counter[20];

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

	// assign VALID_ADDRESS = ~(IACK & ~AS & BOOT);
	// assign VALID_ADDRESS = ~(~AS & BOOT);


endmodule
