module companion(
	input CS,
	input DS,
	input RW,
	inout [7:0] DATA,
	output [2:0] LED
);

reg [7:0] temp = 0;

assign LED[2:0] = temp[2:0];

always @(negedge DS) begin
	if (~CS && ~RW) begin
		temp <= DATA;
	end
end

assign DATA = (~CS && RW) ? temp : 7'bZ;

endmodule
