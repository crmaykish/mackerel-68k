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
	output DTACK,
	output TIMER
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

	// System timer
	
	// Create a periodic interrupt every 65536 clock cycles (~122 Hz at 8MHz clock)
	
	// At each interval, TIMER output will be pulled low until an IACK signal is received
	// If no IACK signal comes (interrupts disabled on the CPU), it will be reset on the next interval
	
	// TODO: Interrupts from other sources will probably not work with IACK connected directly to VPA on the CPU
	// Need some logic to only set VPA when the interrupt is from this periodic timer
	
	reg [15:0] timer_reg = 0;
	reg timer_out = 1;
	reg acked = 0;
	
	always @(posedge CLK) begin
		if (~RST) begin
			timer_reg <= 0;
			timer_out <= 1;
			acked <= 0;
		end
		else begin
			timer_reg <= timer_reg + 1'b1;
			
			if (timer_reg[15] & ~acked) timer_out <= 0;
			
			if (~IACK) begin
				acked <= 1;
				timer_out <= 1;
			end
			
			if (~timer_reg[15]) begin
				acked <= 0;
				timer_out <= 1;
			end
		end
	end

	assign TIMER = timer_out;

	// Define memory map
	
	// 0x380000 - 256K
	assign ROMEN = ~(IACK & ~AS & (~BOOT | (~ADDR[23] & ~ADDR[22] & ADDR[21] & ADDR[20] & ADDR[19] & ~ADDR[18])));
	
	// 0x3C0000 - 256K
	assign MFPEN = ~(IACK & ~AS & BOOT & ~ADDR[23] & ~ADDR[22] & ADDR[21] & ADDR[20] & ADDR[19] & ADDR[18]);
	
	assign RAMEN = ~(IACK & ~AS & BOOT);

	// Generate DTACK signal
	assign DTACK = (MFPEN & DTACK_IN & ~IACK) | (~MFPEN & DTACK_IN & IACK);
	
endmodule
