// Testbench for sdram_cache.v -- exercises miss/fill, temporal + spatial hits,
// and write-through invalidation against a simple latency model of the controller.
`timescale 1ns/1ps
`default_nettype none

module sdram_cache_tb;
    reg clk = 0, rst_n = 0;
    always #5 clk = ~clk;   // 100 MHz

    // CPU-side bus
    reg         cs_n = 1, rwn = 1, udsn = 1, ldsn = 1;
    reg  [22:1] addr = 0;
    wire [15:0] rdata;
    wire        dtack_n;

    // cache <-> controller
    wire        mem_cs_n;
    reg  [15:0] mem_rdata;
    reg  [31:0] mem_rdata32;
    reg         mem_dtack_n = 1;
    reg  [15:0] cpu_word;           // value the CPU is writing (driven by the write task)

    sdram_cache dut (
        .clk(clk), .rst_n(rst_n),
        .cs_n(cs_n), .addr(addr), .rwn(rwn), .udsn(udsn), .ldsn(ldsn),
        .rdata(rdata), .dtack_n(dtack_n),
        .mem_cs_n(mem_cs_n), .mem_rdata(mem_rdata), .mem_rdata32(mem_rdata32),
        .mem_dtack_n(mem_dtack_n)
    );

    // --- simple controller model: 32-bit line memory, fixed read/write latency ---
    localparam LAT = 6;
    reg [31:0] mem [0:1023];        // indexed by line = addr[22:2] (low bits)
    integer    rd_cmds = 0;         // count controller READ commands (to detect hits)
    integer    lat_cnt;
    reg        busy = 0;

    wire [9:0] lineidx = {addr[10:2]};   // small model: line index from low addr bits

    always @(posedge clk) begin
        if (!mem_cs_n && !busy && mem_dtack_n) begin
            busy <= 1; lat_cnt <= LAT;
            if (rwn) rd_cmds <= rd_cmds + 1;
        end else if (busy) begin
            if (lat_cnt > 1) lat_cnt <= lat_cnt - 1;
            else begin
                // complete
                if (!rwn) begin
                    // word write: place word at addr[1] into the 32-bit line, matching the
                    // adapter byte order: word0={d[7:0],d[15:8]}, word1={d[23:16],d[31:24]}
                    if (addr[1]) mem[lineidx][31:16] <= {cpu_word[7:0], cpu_word[15:8]};
                    else         mem[lineidx][15:0]  <= {cpu_word[7:0], cpu_word[15:8]};
                end
                mem_rdata32 <= mem[lineidx];
                mem_rdata   <= addr[1] ? {mem[lineidx][23:16], mem[lineidx][31:24]}
                                       : {mem[lineidx][7:0],   mem[lineidx][15:8]};
                mem_dtack_n <= 0;
                busy <= 0;
            end
        end
        if (mem_cs_n) mem_dtack_n <= 1;   // drop ack when cache releases cs
    end

    // --- CPU bus tasks ---
    integer errors = 0;
    integer prev_rd;

    task do_read(input [22:1] a, input [15:0] exp, input expect_hit);
        begin
            prev_rd = rd_cmds;
            @(negedge clk); cs_n=0; rwn=1; udsn=0; ldsn=0; addr=a;
            wait (dtack_n == 0); @(negedge clk);
            if (rdata !== exp) begin
                $display("FAIL read @%h = %h, expected %h", a, rdata, exp); errors=errors+1;
            end else $display("ok   read @%h = %h", a, rdata);
            if (expect_hit && rd_cmds != prev_rd) begin
                $display("FAIL expected HIT @%h but a controller read happened", a); errors=errors+1;
            end
            if (!expect_hit && rd_cmds == prev_rd) begin
                $display("FAIL expected MISS @%h but no controller read", a); errors=errors+1;
            end
            cs_n=1; udsn=1; ldsn=1; @(negedge clk); @(negedge clk);
        end
    endtask

    task do_write(input [22:1] a, input [15:0] val);
        begin
            cpu_word = val;
            @(negedge clk); cs_n=0; rwn=0; udsn=0; ldsn=0; addr=a;
            wait (dtack_n == 0); @(negedge clk);
            $display("ok   write @%h = %h", a, val);
            cs_n=1; udsn=1; ldsn=1; rwn=1; @(negedge clk); @(negedge clk);
        end
    endtask

    integer k;
    initial begin
        // Word address = (line_index << 1) | word_sel.  Controller read of word0 yields
        // {line[7:0],line[15:8]}, word1 yields {line[23:16],line[31:24]}.
        for (k=0;k<1024;k=k+1) mem[k]=32'h0;
        mem[5][15:0]  = {8'h11, 8'h11};   // line5 word0 -> 0x1111
        mem[5][31:16] = {8'h22, 8'h22};   // line5 word1 -> 0x2222
        mem[9][15:0]  = {8'hAB, 8'hCD};   // line9 word0 -> 0xCDAB

        repeat (4) @(negedge clk); rst_n = 1; repeat (2) @(negedge clk);

        do_read((5<<1)|0, 16'h1111, 0);   // miss, fill line5
        do_read((5<<1)|0, 16'h1111, 1);   // temporal hit
        do_read((5<<1)|1, 16'h2222, 1);   // spatial hit (other word, same line)
        do_read((9<<1)|0, 16'hCDAB, 0);   // different line: miss

        do_write((5<<1)|0, 16'h7777);     // write-through + invalidate line5
        do_read((5<<1)|0, 16'h7777, 0);   // miss (invalidated), returns new value + refills line
        do_read((5<<1)|1, 16'h2222, 1);   // word1 hits: the word0 miss refilled the whole line

        if (errors==0) $display("ALL CACHE TESTS PASS");
        else $display("CACHE TESTS FAILED: %0d errors", errors);
        $finish;
    end

    initial begin #200000; $display("TIMEOUT"); $finish; end
endmodule

`default_nettype wire
