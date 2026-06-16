// Mackerel-F Programmable Timer  -- system-tick interrupt source
//
// A lightweight memory-mapped timer for the Mackerel-F SoC. It raises a
// periodic, level-sensitive interrupt meant to drive the Linux system tick.
// Software can enable/disable it and pick one of four tick rates
// (10 / 25 / 50 / 100 Hz) at run time -- no reflash. Slightly fancier than the
// bare on/off timers on Mackerel-10/30, but still tiny.
//
// It is an 8-bit slave on the upper data lane (like the other Mackerel-F
// peripherals); the port list mirrors uart.v's so wiring it into mackerel_f.v
// later is a copy of the UART slot (add a cs_timer_n decode + route dtack/irq).
//
// Register map (byte offsets within the slot; reg_addr = ADDR_BUS[3:1]):
//   0x00  CTRL    R/W   [0]   ENABLE  1 = counter runs and raises IRQ on rollover
//                       [5:4] FREQ    0 = 10 Hz, 1 = 25 Hz, 2 = 50 Hz, 3 = 100 Hz
//   0x02  STATUS  R     [0]   PENDING 1 = tick pending (IRQ asserted)
//   0x02  ACK     W     write (any value) clears PENDING (acknowledge the tick)
//
// IRQ is level-sensitive: it stays high until software writes ACK, matching the
// 68000 IPL handshake (the ISR acks the tick before RTE). Disabling the timer
// (CTRL.ENABLE = 0) also clears any pending tick and resets the divider, so a
// later re-enable starts a fresh, full-length interval.
//
// The four tick periods are derived from CLK_HZ, so they stay correct if the
// SoC clock changes -- update the one parameter, like the UART baud divisor.
// At clk_soc = 64.8 MHz all four rates divide exactly (no rounding error).

`default_nettype none

module timer #(
    parameter integer CLK_HZ = 64_800_000   // clk frequency; tick periods derive from this
) (
    input  wire       clk,
    input  wire       rst_n,

    // 68k bus (8-bit, upper lane) -- same slave shape as uart.v
    input  wire       cs_n,
    input  wire [2:0] reg_addr,   // ADDR_BUS[3:1]
    input  wire       rwn,
    input  wire       ds_n,
    input  wire [7:0] data_in,
    output reg  [7:0] data_out,
    output wire       dtack_n,

    output wire       irq          // level: high while a tick is pending
);

    // ---- register offsets (reg_addr = byte offset >> 1) ----
    localparam [2:0] REG_CTRL   = 3'd0;   // 0x00  CTRL
    localparam [2:0] REG_STATUS = 3'd1;   // 0x02  STATUS (read) / ACK (write)

    // ---- per-rate reload values, derived from CLK_HZ ----
    // The counter counts clk cycles 0..reload then rolls over, so the tick
    // period is (reload + 1) cycles = CLK_HZ / rate.
    localparam integer DIV_10  = CLK_HZ/10  - 1;
    localparam integer DIV_25  = CLK_HZ/25  - 1;
    localparam integer DIV_50  = CLK_HZ/50  - 1;
    localparam integer DIV_100 = CLK_HZ/100 - 1;

    // counter width sized to the slowest tick (largest divider = 10 Hz)
    localparam integer CW = $clog2(DIV_10 + 1);

    // ---- config + state ----
    reg            enable;
    reg  [1:0]     freq;
    reg  [CW-1:0]  count;
    reg            pending;

    // reload value selected by the current FREQ field
    reg  [CW-1:0]  reload;
    always @(*) begin
        case (freq)
            2'd0:    reload = DIV_10 [CW-1:0];
            2'd1:    reload = DIV_25 [CW-1:0];
            2'd2:    reload = DIV_50 [CW-1:0];
            default: reload = DIV_100[CW-1:0];
        endcase
    end

    // ---- bus: zero wait states, respond as soon as selected ----
    assign dtack_n = cs_n;

    wire wr      = ~cs_n & ~rwn & ~ds_n;            // upper-byte write strobe
    wire wr_ctrl = wr & (reg_addr == REG_CTRL);
    wire wr_ack  = wr & (reg_addr == REG_STATUS);

    // ---- register reads (combinational) ----
    always @(*) begin
        case (reg_addr)
            REG_CTRL:   data_out = {2'b00, freq, 3'b000, enable};
            REG_STATUS: data_out = {7'b0, pending};
            default:    data_out = 8'h00;
        endcase
    end

    assign irq = pending;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            enable  <= 1'b0;
            freq    <= 2'd3;            // default 100 Hz (typical Linux HZ)
            count   <= {CW{1'b0}};
            pending <= 1'b0;
        end else begin
            // free-running divider: roll over every (reload + 1) cycles.
            // >= (not ==) so shortening FREQ mid-interval can't wrap the
            // counter all the way around -- it ticks on the next cycle instead.
            if (enable) begin
                if (count >= reload) begin
                    count   <= {CW{1'b0}};
                    pending <= 1'b1;       // raise the tick
                end else begin
                    count <= count + 1'b1;
                end
            end else begin
                count <= {CW{1'b0}};
            end

            // ACK (write to STATUS) clears the pending tick. Placed after the
            // counter so an ack wins a same-cycle collision with a rollover --
            // a written ack always takes effect; the next rollover re-raises it.
            if (wr_ack)
                pending <= 1'b0;

            // CTRL write. Disabling is authoritative: it clears any pending
            // tick and resets the divider so a re-enable starts a full interval.
            if (wr_ctrl) begin
                enable <= data_in[0];
                freq   <= data_in[5:4];
                if (!data_in[0]) begin
                    count   <= {CW{1'b0}};
                    pending <= 1'b0;
                end
            end
        end
    end

endmodule

`default_nettype wire
