module dram_controller_tb;

reg reset = 1;
reg clk = 0;
reg [23:0] address = 0;
reg as = 1;
reg lds = 1;
reg uds = 1;

wire [10:0] addr_out = 11'b0;
wire ras, cas0, cas1;
wire oe;
wire dtack;

// Start clock
always #1 clk = !clk;

initial begin
    $dumpfile("test.vcd");
    $dumpvars(0, dram_controller_tb);

    // Power on reset
    #10 reset  = 0;
    #10 reset = 1;

    // 16-bit memory access cycle
    #27 address = 24'h120034;
    #2 as = 0;
    #1 lds = 0;
    #1 uds = 0;
    #15 as = 1;
    #1 lds = 1;
    #1 uds = 1;

    // TODO: simulate 8-bit memory cycle

    // TODO: simulate memory cycle during refresh

    #100 $finish;
end

dram_controller dram1(
    .CLK(clk),
    .RST(reset),
    .AS(as),
    .LDS(lds),
    .UDS(uds),
    .ADDR_IN(address),
    .ADDR_OUT(addr_out),
    .RAS(ras),
    .CAS_LOWER(cas0),
    .CAS_UPPER(ca1),
    .OE(oe),
    .DTACK_DRAM(dtack)
);

endmodule
