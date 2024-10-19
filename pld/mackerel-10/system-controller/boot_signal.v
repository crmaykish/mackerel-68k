// BOOT signal is active HIGH indicated the first four bus cycles after reset have completed
module boot_signal(
    input RESET_n,
    input AS_n,
    output reg BOOT = 1'b0
);

localparam BOOT_CYCLE_MAX = 4;

reg [2:0] bus_cycles = 0;

always @(posedge AS_n) begin
	if (~RESET_n) begin 
		bus_cycles = 0;
		BOOT <= 1'b0;
	end
	else begin
		if (~BOOT) begin
			bus_cycles <= bus_cycles + 3'b1;
			if (bus_cycles == BOOT_CYCLE_MAX) BOOT <= 1'b1;
		end
	end
end

endmodule
