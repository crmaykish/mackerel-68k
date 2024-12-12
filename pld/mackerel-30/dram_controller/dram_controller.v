module dram_controller(
	input RST_n,
	input CLK,
	input CLK_CPU,
	
	input CS_n,
	
	input RW,
	
	input SIZ0, SIZ1,
	
	input AS_n, DS_n,
	
	output reg DRAM_WR_n,
	
	input [27:0] ADDR,
	
	output reg [11:0] ADDR_DRAM = 12'b0,
	
	output reg RAS0_n, RAS1_n, RAS2_n, RAS3_n,
	output reg CAS0_n, CAS1_n, CAS2_n, CAS3_n,

	output reg DSACK0_DRAM_n,
	output reg DSACK1_DRAM_n
);

// Assuming 32ms total refresh time at 50 MHz clock
localparam REFRESH_CYCLE_CNT = 781;

// DRAM controller states
localparam IDLE 				= 4'd0;
localparam RW1					= 4'd1;
localparam RW2					= 4'd2;
localparam RW3					= 4'd3;
localparam RW4					= 4'd4;
localparam RW5					= 4'd5;
localparam REFRESH1 			= 4'd6;
localparam REFRESH2				= 4'd7;
localparam REFRESH3				= 4'd8;
localparam REFRESH4				= 4'd9;
localparam PRECHARGE			= 4'd10;

reg [3:0] state = IDLE;

// ==== Periodic refresh generator
reg refresh_request = 1'b0;
reg refresh_ack = 1'b0;
reg [11:0] cycle_count = 12'b0;

always @(posedge CLK) begin
	if (~RST_n) cycle_count <= 12'b0;
	else begin
		cycle_count <= cycle_count + 12'b1;

		if (cycle_count == REFRESH_CYCLE_CNT) begin
			refresh_request <= 1'b1;
			cycle_count <= 12'b0;
		end
		
		if (refresh_ack) refresh_request <= 1'b0;
	end
end

reg AS1_n = 1;
reg CS1_n = 1;
reg AS2_n = 1;
reg CS2_n = 1;

always @(posedge CLK) begin
	AS1_n <= AS_n;
	CS1_n <= CS_n;
	AS2_n <= AS1_n;
	CS2_n <= CS1_n;
end

// ==== DRAM controller state machine
always @(posedge CLK) begin
	if (~RST_n) begin
		state <= IDLE;
		RAS0_n <= 1'b1;
		RAS1_n <= 1'b1;
		RAS2_n <= 1'b1;
		RAS3_n <= 1'b1;
		CAS0_n <= 1'b1;
		CAS1_n <= 1'b1;
		CAS2_n <= 1'b1;
		CAS3_n <= 1'b1;
		DRAM_WR_n <= 1'b1;
		DSACK0_DRAM_n <= 1'b1;
		DSACK1_DRAM_n <= 1'b1;
	end
	else begin
		case (state)
			IDLE: begin
				if (refresh_request) begin
					// Start CAS-before-RAS refresh cycle
					state <= REFRESH1;
				end
				// else if (~CS2 && ~AS2) begin
				// 	// DRAM selected, start normal R/W cycle
				// 	state <= RW1;
				// end
			end

			// RW1: begin
			// 	// Mux in the address
			// 	ADDR_OUT <= ADDR_IN[11:1];
			// 	state <= RW2;
			// end

			// RW2: begin
			// 	// Row address is valid, lower RAS
			// 	if (BANK_A) RASA <= 1'b0;
			// 	else RASB <= 1'b0;
			// 	state <= RW3;
			// end

			// RW3: begin
			// 	// Mux in the column address
			// 	ADDR_OUT <= ADDR_IN[22:12];

			// 	// Set the WE line
			// 	if (BANK_A) WRA <= RW;
			// 	else WRB <= RW;

			// 	state <= RW4;
			// end

			// RW4: begin
			// 	// Column address is valid, lower CAS
			// 	if (BANK_A) begin
			// 		CASA0 <= LDS;
			// 		CASA1 <= UDS;
			// 	end
			// 	else begin
			// 		CASB0 <= LDS;
			// 		CASB1 <= UDS;
			// 	end
			// 	state <= RW5;
			// end

			// RW5: begin
			// 	// Data is valid, lower DTACK
			// 	DTACK_DRAM <= 1'b0;

			// 	// When AS returns high, the bus cycle is complete
			// 	if (AS_n) state <= PRECHARGE;
			// end

			REFRESH1: begin
				// Acknowledge the refresh request
				refresh_ack <= 1'b1;

				// Lower CAS
				CAS0_n <= 1'b0;
				CAS1_n <= 1'b0;
				CAS2_n <= 1'b0;
				CAS3_n <= 1'b0;
				DRAM_WR_n <= 1'b1;
				state <= REFRESH2;
			end
			
			REFRESH2: begin
				// Lower RAS
				RAS0_n <= 1'b0;
				RAS1_n <= 1'b0;
				RAS2_n <= 1'b0;
				RAS3_n <= 1'b0;
				state <= REFRESH3;
			end

			REFRESH3: begin
				// Raise CAS
				CAS0_n <= 1'b1;
				CAS1_n <= 1'b1;
				CAS2_n <= 1'b1;
				CAS3_n <= 1'b1;
				state <= REFRESH4;
			end
			
			REFRESH4: begin
				// Raise RAS
				RAS0_n <= 1'b1;
				RAS1_n <= 1'b1;
				RAS2_n <= 1'b1;
				RAS3_n <= 1'b1;
				state <= PRECHARGE;
			end
			
			PRECHARGE: begin
				// Reset the refresh acknowledge to allow the next refresh cycle
				refresh_ack <= 1'b0;

				// DRAM cycle finished, bring RAS and CAS HIGH
				DSACK0_DRAM_n <= 1'b1;
				DSACK1_DRAM_n <= 1'b1;
				RAS0_n <= 1'b1;
				RAS1_n <= 1'b1;
				RAS2_n <= 1'b1;
				RAS3_n <= 1'b1;
				CAS0_n <= 1'b1;
				CAS1_n <= 1'b1;
				CAS2_n <= 1'b1;
				CAS3_n <= 1'b1;
				ADDR_DRAM <= 12'b0;

				state <= IDLE;
			end
		endcase
	end
end

endmodule
