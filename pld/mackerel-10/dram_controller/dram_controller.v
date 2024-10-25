module dram_controller(
	input CLK,
	input RST,
	input AS,
	input LDS,
	input UDS,
	input RW,
	input CS,	// DRAM chip-select
	input [23:1] ADDR_IN,

	output ADDR_OUT_11,
	
	output reg [10:0] ADDR_OUT = 11'b0,	// Note: this implies 4MB SIMMs
	output reg RASA = 1'b1,
	output reg RASB = 1'b1,
	output reg CASA0 = 1'b1,
	output reg CASA1 = 1'b1,
	output reg CASB0 = 1'b1,
	output reg CASB1 = 1'b1,
	output reg WRA,
	output reg WRB,
	output reg DTACK_DRAM = 1'b1
);

// Assuming 32ms total refresh time at 20 MHz clock
localparam REFRESH_CYCLE_CNT = 312;

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

assign ADDR_OUT_11 = 1'b0;	// A11 is not used on 4MB SIMMs

wire BANK_A = ~ADDR_IN[23];	// Bank A is the bottom 8 MB of DRAM, Bank B the top 8 MB

// ==== Periodic refresh generator
reg refresh_request = 1'b0;
reg refresh_ack = 1'b0;
reg [8:0] cycle_count = 9'b0;

always @(posedge CLK) begin
	if (~RST) cycle_count <= 9'b0;
	else begin
		cycle_count <= cycle_count + 9'b1;

		if (cycle_count == REFRESH_CYCLE_CNT) begin
			refresh_request <= 1'b1;
			cycle_count <= 9'b0;
		end
		
		if (refresh_ack) refresh_request <= 1'b0;
	end
end

// ==== DRAM controller state machine
always @(posedge CLK) begin
	if (~RST) begin
		state <= IDLE;
		RASA <= 1'b1;
		CASA0 <= 1'b1;
		CASA1 <= 1'b1;
		RASB <= 1'b1;
		CASB0 <= 1'b1;
		CASB1 <= 1'b1;
		WRA <= 1'b1;
		WRB <= 1'b1;
		DTACK_DRAM <= 1'b1;
	end
	else begin
		case (state)
			IDLE: begin
				if (refresh_request) begin
					// Start CAS-before-RAS refresh cycle
					state <= REFRESH1;
				end
				else if (~CS && ~AS) begin
					// DRAM selected, start normal R/W cycle
					state <= RW1;
				end
			end

			RW1: begin
				// Mux in the address
				ADDR_OUT <= ADDR_IN[11:1];
				state <= RW2;
			end

			RW2: begin
				// Row address is valid, lower RAS
				if (BANK_A) RASA <= 1'b0;
				else RASB <= 1'b0;
				state <= RW3;
			end

			RW3: begin
				// Mux in the column address
				ADDR_OUT <= ADDR_IN[22:12];

				// Set the WE line
				if (BANK_A) WRA <= RW;
				else WRB <= RW;

				state <= RW4;
			end

			RW4: begin
				// Column address is valid, lower CAS
				if (BANK_A) begin
					CASA0 <= LDS;
					CASA1 <= UDS;
				end
				else begin
					CASB0 <= LDS;
					CASB1 <= UDS;
				end
				state <= RW5;
			end

			RW5: begin
				// Data is valid, lower DTACK
				DTACK_DRAM <= 1'b0;

				// When AS returns high, the bus cycle is complete
				if (AS) state <= PRECHARGE;
			end

			REFRESH1: begin
				// Acknowledge the refresh request
				refresh_ack <= 1'b1;

				// Lower CAS
				CASA0 <= 1'b0;
				CASA1 <= 1'b0;
				CASB0 <= 1'b0;
				CASB1 <= 1'b0;
				WRA <= 1'b1;
				WRB <= 1'b1;
				state <= REFRESH2;
			end
			
			REFRESH2: begin
				// Lower RAS
				RASA <= 1'b0;
				RASB <= 1'b0;
				state <= REFRESH3;
			end

			REFRESH3: begin
				// Raise CAS
				CASA0 <= 1'b1;
				CASA1 <= 1'b1;
				CASB0 <= 1'b1;
				CASB1 <= 1'b1;
				state <= REFRESH4;
			end
			
			REFRESH4: begin
				// Raise RAS
				RASA <= 1'b1;
				RASB <= 1'b1;
				state <= PRECHARGE;
			end
			
			PRECHARGE: begin
				// DRAM cycle finished, bring RAS and CAS HIGH
				DTACK_DRAM <= 1'b1;
				RASA <= 1'b1;
				CASA0 <= 1'b1;
				CASA1 <= 1'b1;
				RASB <= 1'b1;
				CASB0 <= 1'b1;
				CASB1 <= 1'b1;
				ADDR_OUT <= 11'b0;
				refresh_ack <= 1'b0;
				state <= IDLE;
			end
		endcase
	end
end

endmodule
