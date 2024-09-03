module dram_controller(
	input CLK,
	input RST,
	input AS,
	input LDS, UDS,
	input RW,
	input [23:0] ADDR_IN,
	output reg [10:0] ADDR_OUT = 11'b0,	// Note: this implies 4MB SIMMs, Use 11:0 for 4MB
	output reg RAS = 1'b1,
	output reg CAS_LOWER = 1'b1,
	output reg CAS_UPPER = 1'b1,
	output reg WE = 1'b1,
	output OE,
	output reg DTACK_DRAM = 1'b1
);

localparam REFRESH_CYCLE_CNT = 100;	// TODO: calculate this based on the clock rate and the DRAM timing requirements

// DRAM controller states
localparam IDLE 			= 3'd0;
localparam ROW_SELECT1		= 3'd1;
localparam ROW_SELECT2		= 3'd2;
localparam COL_SELECT1		= 3'd3;
localparam COL_SELECT2		= 3'd4;
localparam NEEDS_REFRESH 	= 3'd5;
localparam REFRESH			= 3'd6;
localparam REFRESH_DONE		= 3'd7;

// TODO: Address multplexing can probably happen asynchronously, so it's ready whenever
// the DRAM is selected

// TODO: set up correct refresh cycle time based on clock speed


// DRAM chip-select, 8MB of address space, active LOW
wire CE = ~(~AS && ADDR_IN >= 24'h100000 && ADDR_IN < 24'h900000);

reg [11:0] cycle_count = 12'b0;
reg [2:0] state = IDLE;

// Tie output-enable to LOW
assign OE = 1'b0;	// TODO: set this as part of the DRAM access process?

always @(posedge CLK) begin
	if (~RST) begin
		cycle_count <= 12'b0;
		state <= IDLE;
		RAS <= 1'b1;
		CAS_LOWER <= 1'b1;
		CAS_UPPER <= 1'b1;
		DTACK_DRAM <= 1'b1;
	end
	else begin
		cycle_count <= cycle_count + 12'b1;
		
		// DRAM state machine
		case (state)
			IDLE: begin
				if (cycle_count > REFRESH_CYCLE_CNT) begin
					// Time to run a refresh cycle
					// Reset the counter and set state to NEEDS_REFRESH
					cycle_count <= 12'b0;
					state <= NEEDS_REFRESH;
					WE <= 1'b1;
				end
				else if (~CE) begin
					// DRAM is selected by the CPU, start the access process
					ADDR_OUT <= ADDR_IN[10:0];
					WE <= RW;
					state <= ROW_SELECT1;
				end
			end
			
			ROW_SELECT1: begin
				// Lower RAS to latch in the row address
				RAS <= 1'b0;
				state <= ROW_SELECT2;
			end

			ROW_SELECT2: begin
				// Set the DRAM address to the column address
				ADDR_OUT <= ADDR_IN[21:11];
				state <= COL_SELECT1;
			end

			COL_SELECT1: begin
				// Lower CAS to latch in the column address
				CAS_LOWER <= LDS;
				CAS_UPPER <= UDS;
				state <= COL_SELECT2;
			end

			COL_SELECT2: begin
				// DRAM data is ready, lower DTACK
				DTACK_DRAM <= 1'b0;

				// Wait for AS to go HIGH
				if (AS) begin
					// CPU memory cycle is complete, reset DRAM signals
					RAS <= 1'b1;

					// TODO: Does there need to be a delay between raising CAS and raising RAS?

					CAS_LOWER <= 1'b1;
					CAS_UPPER <= 1'b1;
					DTACK_DRAM <= 1'b1;
					WE <= 1'b1;
					ADDR_OUT <= 12'b0;	// TODO this might not be necessary
					state <= IDLE;
				end
			end

			NEEDS_REFRESH: begin
				// Lower CAS
				CAS_LOWER <= 1'b0;
				CAS_UPPER <= 1'b0;
				state <= REFRESH;
			end
			
			REFRESH: begin
				// Lower RAS
				RAS <= 1'b0;
				state <= REFRESH_DONE;
			end

			REFRESH_DONE: begin
				// Refresh cycle finished, bring RAS and CAS HIGH
				RAS <= 1'b1;
				CAS_LOWER <= 1'b1;
				CAS_UPPER <= 1'b1;
				state <= IDLE;
			end
		endcase
	end
end

endmodule
