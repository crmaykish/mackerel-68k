// sdram_controller.v - 8 MB SDRAM adapter for Mackerel-F (Tang Nano 20k in-package SDRAM).
//
// Wraps nand2mario's open byte-addressable controller and bridges
// it to the 68000 asynchronous bus. The controller runs in the SAME clock domain

`default_nettype none

module sdram_controller (
    input  wire        clk,        // 75.6 MHz, shared with the 68k bus (= clk_soc)
    input  wire        clk_ps,     // 75.6 MHz phase-shifted -> SDRAM chip clock
    input  wire        rst_n,

    // 68k bus side (clk domain)
    input  wire        cs_n,
    input  wire [22:1] addr,
    input  wire        rwn,
    input  wire        udsn,
    input  wire        ldsn,
    input  wire [15:0] wdata,
    output reg  [15:0] rdata,
    output reg  [31:0] rdata32,    // raw 32-bit line (both words) for the cache fill
    output wire        dtack_n,
    output reg         init_done,

    // SDRAM physical side -> top-level ports
    output wire        O_sdram_clk,
    output wire        O_sdram_cke,
    output wire        O_sdram_cs_n,
    output wire        O_sdram_cas_n,
    output wire        O_sdram_ras_n,
    output wire        O_sdram_wen_n,
    output wire [3:0]  O_sdram_dqm,
    output wire [10:0] O_sdram_addr,
    output wire [1:0]  O_sdram_ba,
    inout  wire [31:0] IO_sdram_dq
);

    // ---- nand2mario byte-addressable controller (same clock domain as the bus) ----
    reg         ctrl_rd, ctrl_wr, ctrl_refresh;
    reg  [22:0] ctrl_addr;
    reg  [7:0]  ctrl_din;
    wire [31:0] ctrl_dout32;
    wire        ctrl_busy, ctrl_data_ready;

    sdram#(.FREQ(75_600_000)) u_ctrl (
        .clk(clk),
        .clk_sdram(clk_ps),
        .resetn(rst_n),
        .rd(ctrl_rd), .wr(ctrl_wr), .refresh(ctrl_refresh),
        .addr(ctrl_addr), .din(ctrl_din),
        .dout(), .dout32(ctrl_dout32),
        .data_ready(ctrl_data_ready), .busy(ctrl_busy),
        .SDRAM_DQ(IO_sdram_dq),
        .SDRAM_A(O_sdram_addr),
        .SDRAM_BA(O_sdram_ba),
        .SDRAM_nCS(O_sdram_cs_n),
        .SDRAM_nWE(O_sdram_wen_n),
        .SDRAM_nRAS(O_sdram_ras_n),
        .SDRAM_nCAS(O_sdram_cas_n),
        .SDRAM_CLK(O_sdram_clk),
        .SDRAM_CKE(O_sdram_cke),
        .SDRAM_DQM(O_sdram_dqm)
    );

    // ---- refresh timer: one auto-refresh every ~5 us (>= 4096 per 64 ms) ----
    localparam [8:0] REFRESH_INT = 9'd340;   // ~4.5 us @ 75.6 MHz (well under the 15.6 us max)
    reg [8:0] ref_cnt;
    reg       ref_due;
    reg       init_seen;
    reg       need_second;

    // ---- single FSM (clk domain): 68k bus handshake + controller commands ----
    localparam [2:0] S_IDLE=3'd0, S_RD=3'd1, S_WR=3'd2, S_REF=3'd3, S_ACK=3'd4;
    reg [2:0] state;
    reg       dtack;
    assign dtack_n = ~dtack;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= S_IDLE; ctrl_rd <= 1'b0; ctrl_wr <= 1'b0; ctrl_refresh <= 1'b0;
            ctrl_addr <= 23'd0; ctrl_din <= 8'd0; rdata <= 16'd0; rdata32 <= 32'd0; dtack <= 1'b0;
            ref_cnt <= 9'd0; ref_due <= 1'b0; init_seen <= 1'b0; init_done <= 1'b0;
            need_second <= 1'b0;
        end else begin
            ctrl_rd <= 1'b0; ctrl_wr <= 1'b0; ctrl_refresh <= 1'b0;  // 1-cycle pulses

            if (!ctrl_busy) begin init_seen <= 1'b1; init_done <= 1'b1; end
            if (init_seen) begin
                if (ref_cnt >= REFRESH_INT) begin ref_due <= 1'b1; ref_cnt <= 9'd0; end
                else ref_cnt <= ref_cnt + 1'b1;
            end

            case (state)
                S_IDLE: if (init_seen && !ctrl_busy) begin
                    if (ref_due) begin
                        ctrl_refresh <= 1'b1; ref_due <= 1'b0; state <= S_REF;
                    // act only once a data strobe is asserted (write data valid by then)
                    end else if (!cs_n && (!udsn || !ldsn)) begin
                        if (rwn) begin
                            ctrl_rd <= 1'b1; ctrl_addr <= {addr[22:1], 1'b0}; state <= S_RD;
                        end else if (!udsn) begin
                            ctrl_wr <= 1'b1; ctrl_addr <= {addr[22:1], 1'b0};
                            ctrl_din <= wdata[15:8]; need_second <= ~ldsn; state <= S_WR;
                        end else begin
                            ctrl_wr <= 1'b1; ctrl_addr <= {addr[22:1], 1'b1};
                            ctrl_din <= wdata[7:0]; need_second <= 1'b0; state <= S_WR;
                        end
                    end
                end

                // !ctrl_rd/!ctrl_wr/!ctrl_refresh: don't sample busy until the
                // command pulse has cleared (busy only rises the cycle after it).
                S_RD: begin
                    if (ctrl_data_ready) begin
                        rdata <= addr[1] ? {ctrl_dout32[23:16], ctrl_dout32[31:24]}
                                         : {ctrl_dout32[7:0],   ctrl_dout32[15:8]};
                        rdata32 <= ctrl_dout32;
                    end
                    if (!ctrl_rd && !ctrl_busy) begin dtack <= 1'b1; state <= S_ACK; end
                end

                S_WR: if (!ctrl_wr && !ctrl_busy) begin
                    if (need_second) begin                       // now the LDS byte
                        ctrl_wr <= 1'b1; ctrl_addr <= {addr[22:1], 1'b1};
                        ctrl_din <= wdata[7:0]; need_second <= 1'b0;
                    end else begin
                        dtack <= 1'b1; state <= S_ACK;
                    end
                end

                S_REF: if (!ctrl_refresh && !ctrl_busy) state <= S_IDLE;

                S_ACK: if (cs_n) begin dtack <= 1'b0; state <= S_IDLE; end
            endcase
        end
    end

endmodule

`default_nettype wire
