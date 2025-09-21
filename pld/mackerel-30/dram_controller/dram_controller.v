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
localparam RW2A					= 4'd11;
localparam RW4A					= 4'd12;

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

// ==== Double clock input signals from CPU clock domain
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

// ==== Translate cycle type to appropriate CAS signals
// This comes directly from Table 7-4 in the MC68030 datasheet
wire [3:0] CYCLE_TYPE = {SIZ1, SIZ0, ADDR[1], ADDR[0]};
reg [3:0] CAS;	// active high

always @(*) begin
	case (CYCLE_TYPE)
		// CYCLE TYPE <= CAS[3:0]

		// byte
		4'b0100: CAS <= 4'b1000;
		4'b0101: CAS <= 4'b0100;
		4'b0110: CAS <= 4'b0010;
		4'b0111: CAS <= 4'b0001;

		// word
		4'b1000: CAS <= 4'b1100;
		4'b1001: CAS <= 4'b0110;
		4'b1010: CAS <= 4'b0011;
		4'b1011: CAS <= 4'b0001;

		// 3-byte
		4'b1100: CAS <= 4'b1110;
		4'b1101: CAS <= 4'b0111;
		4'b1110: CAS <= 4'b0011;
		4'b1111: CAS <= 4'b0001;

		// long word
		4'b0000: CAS <= 4'b1111;
		4'b0001: CAS <= 4'b0111;
		4'b0010: CAS <= 4'b0011;
		4'b0011: CAS <= 4'b0001;

		default: CAS <= 4'b1111;
	endcase
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
				else if (~CS2_n && ~AS2_n) begin
					// DRAM selected, start normal R/W cycle
					state <= RW1;
				end
			end

			RW1: begin
				// Mux in the address
				ADDR_DRAM <= ADDR[13:2];
				state <= RW2;
			end

			RW2: begin
				// Row address is valid, lower RAS

				// NOTE: This is set up for 64/128 MB SIMMs
				// If A26 is 0, the first side of the SIMM is selected, else the other side is selected

				RAS0_n <= ADDR[26];
				RAS1_n <= ~ADDR[26];
				RAS2_n <= ADDR[26];
				RAS3_n <= ~ADDR[26];

				state <= RW3;
			end

			RW2A: begin
				// Wait state for RAS to settle
				state <= RW3;
			end

			RW3: begin
				// Mux in the column address
				ADDR_DRAM <= ADDR[25:14];

				// Set the WE line
				DRAM_WR_n <= RW;

				state <= RW4;
			end

			RW4: begin
				// Column address is valid, lower CAS
				CAS0_n <= ~CAS[0];
				CAS1_n <= ~CAS[1];
				CAS2_n <= ~CAS[2];
				CAS3_n <= ~CAS[3];

				state <= RW5;
			end

			RW4A: begin
				// Wait state for CAS to settle
				state <= RW5;
			end

			RW5: begin
				// Data is valid, lower DSACK
				
				// TODO: figure out which DSACK pins to assert based on bus cycle width?
				
				DSACK0_DRAM_n <= 1'b0;
				DSACK1_DRAM_n <= 1'b0;

				// When AS returns high, the bus cycle is complete
				if (AS_n) state <= PRECHARGE;
			end

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
