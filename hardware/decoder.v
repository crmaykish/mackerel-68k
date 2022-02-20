module mackerel_decoder(
	input CLK_SRC,
	input RST,
	input [21:15] ADDR,
	input AS,
	output CLK_GEN,
	output ROMEN,
	output RAMEN0,
	output RAMEN1,
	output RAMEN2,
	output RAMEN3,        
	output MFPEN
);
	reg BOOT = 1'b0;
	
	// Generate the CPU clock from the source clock
	reg [3:0] clock_counter = 4'b0;
	always @(posedge CLK_SRC) clock_counter <= clock_counter + 1'b1;
	assign CLK_GEN = clock_counter[3];
	
	reg [3:0] bus_cycles = 0;
	
	reg got_cycle = 1'b0;
	
	always @(posedge CLK_GEN) begin
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
	
	// 0x0000
	assign ROMEN = ~(~AS & ~ADDR[21] & ~ADDR[20] & ~ADDR[19] & ~ADDR[18] & ~ADDR[17] & ~ADDR[16] & ~ADDR[15]);
	
	// 0x8000
	assign MFPEN = ~(~ADDR[21] & ~ADDR[20] & ~ADDR[19] & ~ADDR[18] & ~ADDR[17] & ~ADDR[16] & ADDR[15]);
	
	// 0x380000
	assign RAMEN0 = ~(~AS & ADDR[21] & ADDR[20] & ADDR[19]);
	assign RAMEN1 = 1'b1;
	assign RAMEN2 = 1'b1;
	assign RAMEN3 = BOOT;
	
endmodule
