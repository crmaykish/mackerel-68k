module dram_controller(
	input CLK,
	input CLK_ALT,
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

// Clock cycles between DRAM refreshes
// TODO: confirm the timing requirements and double check this math
localparam REFRESH_CYCLE_CNT = 780;

// DRAM controller states
localparam IDLE 				= 4'd0;
localparam ROW_SELECT1		= 4'd1;
localparam ROW_SELECT2		= 4'd2;
localparam COL_SELECT1		= 4'd3;
localparam COL_SELECT2		= 4'd4;
localparam REFRESH1 			= 4'd5;
localparam REFRESH2			= 4'd6;
localparam REFRESH3			= 4'd7;
localparam REFRESH4			= 4'd8;
localparam PRECHARGE			= 4'd9;

reg [11:0] cycle_count = 12'b0;
reg [3:0] state = IDLE;

assign ADDR_OUT_11 = 1'b0;

reg CS1 = 1;
reg AS1 = 1;

always @(posedge CLK_ALT) begin
	if (~RST) begin
		cycle_count <= 12'b0;
		state <= IDLE;
		RASA <= 1'b1;
		CASA0 <= 1'b1;
		CASA1 <= 1'b1;
		RASB <= 1'b1;
		CASB0 <= 1'b1;
		CASB1 <= 1'b1;
		DTACK_DRAM <= 1'b1;
	end
	else begin
		cycle_count <= cycle_count + 12'b1;
		
		CS1 <= CS;
		AS1 <= AS;
		
		// DRAM state machine
		case (state)
			IDLE: begin
				if (cycle_count > REFRESH_CYCLE_CNT) begin
					// Time to run a refresh cycle
					// Reset the counter and set state to REFRESH1
					cycle_count <= 12'b0;
					state <= REFRESH1;
					WRA <= 1'b1;
					WRB <= 1'b1;
				end
				else if (~CS1 && ~AS1) begin
					// DRAM is selected by the CPU, start the access process
					ADDR_OUT <= ADDR_IN[11:1];
					
					if (~ADDR_IN[23]) WRA <= RW;
					else WRB <= RW;
					
					state <= ROW_SELECT1;
				end
			end

			ROW_SELECT1: begin
				// Lower RAS to latch in the row address
				
				if (~ADDR_IN[23]) RASA <= 1'b0;
				else RASB <= 1'b0;
				
				state <= ROW_SELECT2;
			end

			ROW_SELECT2: begin
				// Set the DRAM address to the column address
				ADDR_OUT <= ADDR_IN[22:12];
				state <= COL_SELECT1;
			end

			COL_SELECT1: begin
				// Lower CAS to latch in the column address
				
				if (~ADDR_IN[23]) begin
					CASA0 <= LDS;
					CASA1 <= UDS;
				end
				else begin
					CASB0 <= LDS;
					CASB1 <= UDS;
				end
				
				state <= COL_SELECT2;
			end

			COL_SELECT2: begin
				// Wait for AS to go HIGH
				if (AS) begin
					// CPU memory cycle is complete, reset DRAM signals
					RASA <= 1'b1;
					RASB <= 1'b1;

					CASA0 <= 1'b1;
					CASA1 <= 1'b1;
					CASB0 <= 1'b1;
					CASB1 <= 1'b1;
					DTACK_DRAM <= 1'b1;
					WRA <= 1'b1;
					WRB <= 1'b1;
					state <= PRECHARGE;
				end
				else begin
					// DRAM data is ready, lower DTACK
					DTACK_DRAM <= 1'b0;
				end
			end

			REFRESH1: begin
				// Lower CAS
				CASA0 <= 1'b0;
				CASA1 <= 1'b0;
				CASB0 <= 1'b0;
				CASB1 <= 1'b0;
				state <= REFRESH2;
			end
			
			REFRESH2: begin
				// Lower RAS
				RASA <= 1'b0;
				RASB <= 1'b0;
				state <= REFRESH3;
			end

			REFRESH3: begin
				// Raise RAS
				RASA <= 1'b1;
				RASB <= 1'b1;
				state <= REFRESH4;
			end
			
			REFRESH4: begin
				// Raise CAS
				CASA0 <= 1'b1;
				CASA1 <= 1'b1;
				CASB0 <= 1'b1;
				CASB1 <= 1'b1;
				state <= PRECHARGE;
			end
			
			PRECHARGE: begin
				// DRAM cycle finished, bring RAS and CAS HIGH
				RASA <= 1'b1;
				CASA0 <= 1'b1;
				CASA1 <= 1'b1;
				RASB <= 1'b1;
				CASB0 <= 1'b1;
				CASB1 <= 1'b1;
				state <= IDLE;
			end
		endcase
	end
end

endmodule
