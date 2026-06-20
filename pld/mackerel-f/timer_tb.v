`timescale 1ns / 1ps
//
// Testbench for the Mackerel-F programmable timer (timer.v).
//
// The DUT is instantiated with a deliberately small CLK_HZ so the four tick
// rates become short, observable periods (the divider math is identical to the
// real 75.6 MHz part -- only the absolute counts shrink). With CLK_HZ = 1000:
//   10 Hz -> 100 cycles, 25 Hz -> 40, 50 Hz -> 20, 100 Hz -> 10.
//
// Checks: reset defaults, CTRL read/write + freq select, the exact tick period
// for every rate, STATUS/ACK behaviour, and that disable stops + clears the IRQ.

module timer_tb;

    localparam integer CLK_HZ = 1000;   // shrink the dividers for a fast sim

    // expected tick period (cycles) for each FREQ code, from the same constant
    localparam integer P_10  = CLK_HZ/10;
    localparam integer P_25  = CLK_HZ/25;
    localparam integer P_50  = CLK_HZ/50;
    localparam integer P_100 = CLK_HZ/100;

    reg        clk = 1'b0;
    reg        rst_n = 1'b0;
    reg        cs_n = 1'b1;
    reg  [2:0] reg_addr = 3'd0;
    reg        rwn = 1'b1;
    reg        ds_n = 1'b1;
    reg  [7:0] data_in = 8'h00;
    wire [7:0] data_out;
    wire       dtack_n;
    wire       irq;

    integer    errors = 0;
    integer    cyc    = 0;       // free-running cycle counter for period math

    timer #(.CLK_HZ(CLK_HZ)) dut (
        .clk(clk), .rst_n(rst_n),
        .cs_n(cs_n), .reg_addr(reg_addr), .rwn(rwn), .ds_n(ds_n),
        .data_in(data_in), .data_out(data_out), .dtack_n(dtack_n),
        .irq(irq)
    );

    always #5 clk = ~clk;                 // 100 MHz sim clock, 10 ns period
    always @(posedge clk) cyc = cyc + 1;

    // ---- bus tasks: drive inputs on negedge so they're stable at the posedge ----
    task bus_write(input [2:0] a, input [7:0] d);
    begin
        @(negedge clk);
        cs_n = 0; rwn = 0; ds_n = 0; reg_addr = a; data_in = d;
        @(negedge clk);                  // the posedge between samples the write
        cs_n = 1; rwn = 1; ds_n = 1;
    end
    endtask

    task bus_read(input [2:0] a, output [7:0] d);
    begin
        @(negedge clk);
        cs_n = 0; rwn = 1; ds_n = 0; reg_addr = a;
        #1 d = data_out;                 // let the combinational read settle, then sample
        @(negedge clk);
        cs_n = 1; ds_n = 1;
    end
    endtask

    // helpers
    localparam [2:0] REG_CTRL   = 3'd0;
    localparam [2:0] REG_STATUS = 3'd1;

    // build a CTRL value: enable + freq code
    function [7:0] ctrl_val(input en, input [1:0] f);
        ctrl_val = {2'b00, f, 3'b000, en};
    endfunction

    task expect_eq(input [255:0] name, input [31:0] got, input [31:0] exp);
    begin
        if (got !== exp) begin
            $display("  FAIL: %0s = %0d, expected %0d", name, got, exp);
            errors = errors + 1;
        end else begin
            $display("  ok:   %0s = %0d", name, got);
        end
    end
    endtask

    // wait for the next rising edge of irq, sampling at negedge (stable)
    task wait_tick;
    begin
        @(negedge clk);
        while (irq)  @(negedge clk);     // ensure it is low first
        while (!irq) @(negedge clk);     // then catch the rising edge
    end
    endtask

    // measure one FREQ code: enable, time tick-to-tick, compare to expected
    task measure(input [1:0] f, input integer expected);
        integer ca, cb;
        reg [7:0] rd;
    begin
        bus_write(REG_CTRL, ctrl_val(1'b1, f));   // enable at rate f
        wait_tick();           ca = cyc;          // tick A
        // STATUS must read pending while the line is asserted
        bus_read(REG_STATUS, rd);
        if (rd[0] !== 1'b1) begin
            $display("  FAIL: STATUS.PENDING not set during IRQ (freq %0d)", f);
            errors = errors + 1;
        end
        bus_write(REG_STATUS, 8'h00);             // ACK clears it
        wait_tick();           cb = cyc;          // tick B
        bus_write(REG_STATUS, 8'h00);             // ACK
        $write("freq %0d:", f);
        expect_eq("tick period (cycles)", cb - ca, expected);
    end
    endtask

    integer i;
    reg [7:0] rd;

    initial begin
        $dumpfile("timer.vcd");
        $dumpvars(0, timer_tb);

        // ---- reset ----
        rst_n = 0;
        repeat (4) @(negedge clk);
        rst_n = 1;
        @(negedge clk);

        $display("== reset defaults ==");
        bus_read(REG_CTRL, rd);
        expect_eq("CTRL after reset", rd, ctrl_val(1'b0, 2'd3));  // disabled, 100 Hz
        bus_read(REG_STATUS, rd);
        expect_eq("STATUS after reset", rd, 8'h00);
        expect_eq("irq after reset", irq, 1'b0);

        // disabled timer must not tick
        $display("== disabled timer is quiet ==");
        repeat (2*P_100 + 5) @(negedge clk);
        expect_eq("irq while disabled", irq, 1'b0);

        // ---- CTRL write/readback ----
        $display("== CTRL write/readback ==");
        bus_write(REG_CTRL, ctrl_val(1'b1, 2'd1));   // enable, 25 Hz
        bus_read(REG_CTRL, rd);
        expect_eq("CTRL readback", rd, ctrl_val(1'b1, 2'd1));
        bus_write(REG_CTRL, ctrl_val(1'b0, 2'd3));   // back to disabled default
        @(negedge clk);

        // ---- per-rate tick periods ----
        $display("== tick periods ==");
        measure(2'd3, P_100);   // 100 Hz
        measure(2'd2, P_50);    //  50 Hz
        measure(2'd1, P_25);    //  25 Hz
        measure(2'd0, P_10);    //  10 Hz

        // ---- ACK behaviour ----
        $display("== ACK clears IRQ ==");
        bus_write(REG_CTRL, ctrl_val(1'b1, 2'd3));   // enable 100 Hz
        wait_tick();
        expect_eq("irq asserted before ACK", irq, 1'b1);
        bus_write(REG_STATUS, 8'h00);                // ACK
        @(negedge clk);
        expect_eq("irq cleared after ACK", irq, 1'b0);
        bus_read(REG_STATUS, rd);
        expect_eq("STATUS.PENDING after ACK", rd, 8'h00);

        // ---- disable stops + clears ----
        $display("== disable stops and clears ==");
        wait_tick();                                 // let it raise another tick
        expect_eq("irq asserted before disable", irq, 1'b1);
        bus_write(REG_CTRL, ctrl_val(1'b0, 2'd3));   // disable
        @(negedge clk);
        expect_eq("irq cleared by disable", irq, 1'b0);
        repeat (2*P_100 + 5) @(negedge clk);         // stays quiet
        expect_eq("irq stays low while disabled", irq, 1'b0);

        // ---- dtack ----
        $display("== dtack ==");
        @(negedge clk); cs_n = 0; ds_n = 0; reg_addr = REG_CTRL; rwn = 1;
        #1 expect_eq("dtack_n asserted when selected", dtack_n, 1'b0);
        @(negedge clk); cs_n = 1; ds_n = 1;
        #1 expect_eq("dtack_n released when idle", dtack_n, 1'b1);

        // ---- summary ----
        $display("");
        if (errors == 0)
            $display("RESULT: ALL TESTS PASSED");
        else
            $display("RESULT: %0d FAILURE(S)", errors);
        $finish;
    end

    // safety net so a hang doesn't run forever
    initial begin
        #2_000_000;
        $display("RESULT: TIMEOUT -- testbench did not finish");
        $finish;
    end

endmodule
