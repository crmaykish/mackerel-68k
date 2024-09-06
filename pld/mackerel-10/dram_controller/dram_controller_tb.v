`timescale 1ns / 1ps

module dram_controller_tb;

reg reset = 1;
reg clk = 0;
reg cs = 1;
reg [23:1] address = 0;
reg as = 1;
reg lds = 1;
reg uds = 1;
reg rw = 1;

wire [10:0] addr_out = 11'b0;
wire ras, cas0, cas1;
wire dtack;

// Start clock
always #25 clk = !clk;

initial begin
    $dumpfile("test.vcd");
    $dumpvars(0, dram_controller_tb);

    // Power on reset
    #1000 reset  = 0;
    #1000 reset = 1;
    #1000

    // 16-bit memory access cycle
    #200 address = 24'h120034;
    cs = 0;
    rw = 1;
    as = 0;
    lds = 0;
    uds = 0;

    #500 as = 1;
    lds = 1;
    uds = 1;
    cs = 1;

    // Wait for a while and expect to see some CBR refreshes

    #100000

    // TODO: simulate 8-bit memory cycle

    // TODO: simulate memory cycle during refresh

    // TODO: simulate write cycle

    #10000 $finish;
end

dram_controller dram1(
    .CLK(clk),
    .RST(reset),
    .CS(cs),
    .AS(as),
    .LDS(lds),
    .UDS(uds),
    .RW(rw),
    .ADDR_IN(address),
    .ADDR_OUT(addr_out),
    .RAS(ras),
    .CAS_LOWER(cas0),
    .CAS_UPPER(ca1),
    .DTACK_DRAM(dtack)
);

endmodule
