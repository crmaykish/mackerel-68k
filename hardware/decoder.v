module mackerel_decoder(
	input CLK,
	input RST,
	input [21:15] ADDR,
	//input A0, A1, A2,
	input FC0, FC1, FC2,
	input AS,
	input DTACK_MFP,
	output CLK_SLOW,
	output ROMEN,
	output RAMEN0,
	output RAMEN1,
	output RAMEN2,
	output RAMEN3,        
	output MFPEN,
	output DTACK,
	output IACK
);
	
	reg [1:0] count_slow = 0;
	
	always @(posedge CLK) begin
		count_slow <= count_slow + 1'b1;
	end
	
	// Secondary clock at 1/2 speed of the CPU clock
	assign CLK_SLOW = count_slow[0];
	
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
	
	// 0x3F8000
	assign ROMEN = ~(IACK & ~AS & (~BOOT | (ADDR[21] & ADDR[20] & ADDR[19] & ADDR[18] & ADDR[17] & ADDR[16] & ADDR[15])));
	
	// 0x3F0000
	assign MFPEN = ~(ADDR[21] & ADDR[20] & ADDR[19] & ADDR[18] & ADDR[17] & ADDR[16] & ~ADDR[15]);
	
	// 512KB SRAM at 0x000000
	assign RAMEN0 = ~(IACK & ~AS & BOOT & ~ADDR[21] & ~ADDR[20] & ~ADDR[19]);
	
	// 512KB SRAM at 0x080000
	assign RAMEN1 = ~(IACK & ~AS & BOOT & ~ADDR[21] & ~ADDR[20] & ADDR[19]);
	
	// 512KB SRAM at 0x100000
	assign RAMEN2 = ~(IACK & ~AS & BOOT & ~ADDR[21] & ADDR[20] & ~ADDR[19]);
	
	// 512KB SRAM at 0x180000
	assign RAMEN3 = ~(IACK & ~AS & BOOT & ~ADDR[21] & ADDR[20] & ADDR[19]);
	
	// Generate DTACK signal
	assign DTACK = (MFPEN & DTACK_MFP & ~IACK) | (~MFPEN & DTACK_MFP & IACK);
	
	// Generate IACK signal
	// NOTE: this will respond to all interrupt levels, eventually this will need to include the A2:A0 interrupt vector to ACK the appropriate interrupt level
	assign IACK = ~(FC0 & FC1 & FC2);
	
endmodule
