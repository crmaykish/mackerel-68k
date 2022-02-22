module mackerel_decoder(
	input CLK_SRC,
	input RST,
	input [21:15] ADDR,
	input AS,
	input DTACK_MFP,
	output CLK_GEN,
	output CLK_SLOW,
	output ROMEN,
	output RAMEN0,
	output RAMEN1,
	output RAMEN2,
	output RAMEN3,        
	output MFPEN,
	output reg DTACK
);
	
	// 5 MHz CPU clock from 50 MHz oscillator
	parameter DIVISOR = 8;
	reg [32:0] counter = 0;

	always @(posedge CLK_SRC) begin
		counter <= counter + 32'd1;
		if(counter >= (DIVISOR-1))
			counter <= 32'd0;
	end
	
	assign CLK_GEN = (counter < DIVISOR / 2) ? 1'b1 : 1'b0;
	
	reg [1:0] count_slow = 0;
	
	always @(posedge CLK_GEN) begin
		count_slow <= count_slow + 1'b1;
	end
	
	// Secondary clock at 1/4 speed of the CPU clock
	assign CLK_SLOW = count_slow[1];
	
	// Generate the BOOT signal for the first 8 memory accesses after reset
	reg BOOT = 1'b0;
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
	
	// Define memory map
	
	// 0x3F8000
	assign ROMEN = ~(~AS & (~BOOT | (ADDR[21] & ADDR[20] & ADDR[19] & ADDR[18] & ADDR[17] & ADDR[16] & ADDR[15])));
	
	// 0x3F0000
	assign MFPEN = ~(ADDR[21] & ADDR[20] & ADDR[19] & ADDR[18] & ADDR[17] & ADDR[16] & ~ADDR[15]);
	
	// 0x000000
	assign RAMEN0 = ~(~AS & BOOT & ~ADDR[21] & ~ADDR[20] & ~ADDR[19]);
	
	assign RAMEN1 = 1'b1;
	assign RAMEN2 = 1'b1;
	assign RAMEN3 = 1'b1;
	
	// Generate DTACK signal
	always @(posedge CLK_GEN) begin
		if (~MFPEN) DTACK <= DTACK_MFP;
		else DTACK <= 1'b0;
	end
	
endmodule
