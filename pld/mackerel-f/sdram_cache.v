// sdram_cache.v - direct-mapped, write-through read cache for the Mackerel-F SDRAM.
//
// Sits between the 68k bus and sdram_controller. The fx68k is the ONLY master that
// writes SDRAM (SD/NIC are SPI/PIO, no DMA), so write-through + invalidate-on-write is
// automatically coherent
//
// 512 lines x 32 bits. A line is one 32-bit-aligned chunk = the two 16-bit words the
// controller already returns per read (dout32), so a line fills in a single SDRAM read.
//
// Address (word address addr[22:1]): [1] = word-in-line, [10:2] = index, [22:11] = tag.

`default_nettype none

module sdram_cache (
    input  wire        clk,
    input  wire        rst_n,

    // 68k bus side
    input  wire        cs_n,
    input  wire [22:1] addr,
    input  wire        rwn,
    input  wire        udsn,
    input  wire        ldsn,
    output reg  [15:0] rdata,
    output wire        dtack_n,

    // memory side
    output reg         mem_cs_n,
    input  wire [15:0] mem_rdata,
    input  wire [31:0] mem_rdata32,
    input  wire        mem_dtack_n
);

    localparam IDX_W = 9;          // 512 lines
    localparam TAG_W = 12;         // addr[22:11]

    reg [31:0]        data_ram [0:(1<<IDX_W)-1];
    reg [TAG_W-1:0]   tag_ram  [0:(1<<IDX_W)-1];
    reg [(1<<IDX_W)-1:0] valid;

    localparam IDLE=3'd0, LOOK=3'd1, FILL=3'd2, WRTH=3'd3, ACK=3'd4;
    reg [2:0] state;

    reg [IDX_W-1:0] idx;
    reg [TAG_W-1:0] reqtag;
    reg             wsel;

    wire             start   = ~cs_n & (~udsn | ~ldsn);
    wire [IDX_W-1:0] cpu_idx = addr[10:2];
    wire [IDX_W-1:0] rd_idx  = (state == IDLE) ? cpu_idx : idx;

    // BSRAM registered read of the line under lookup
    reg [31:0]      data_q;
    reg [TAG_W-1:0] tag_q;
    reg             valid_q;
    always @(posedge clk) begin
        data_q  <= data_ram[rd_idx];
        tag_q   <= tag_ram[rd_idx];
        valid_q <= valid[rd_idx];
    end

    wire        hit = valid_q & (tag_q == reqtag);
    // reconstruct the addressed word from a 32-bit line (same byte order as the adapter)
    wire [15:0] hit_word = wsel ? {data_q[23:16], data_q[31:24]}
                                : {data_q[7:0],   data_q[15:8]};

    reg dtack;
    assign dtack_n = ~dtack;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state    <= IDLE;
            mem_cs_n <= 1'b1;
            dtack    <= 1'b0;
            rdata    <= 16'd0;
            valid    <= {(1<<IDX_W){1'b0}};
            idx      <= {IDX_W{1'b0}};
            reqtag   <= {TAG_W{1'b0}};
            wsel     <= 1'b0;
        end else begin
            case (state)
                IDLE: begin
                    mem_cs_n <= 1'b1;
                    dtack    <= 1'b0;
                    if (start) begin
                        idx    <= cpu_idx;
                        reqtag <= addr[22:11];
                        wsel   <= addr[1];
                        if (rwn) begin
                            state <= LOOK;            // read: look up the tag
                        end else begin
                            mem_cs_n <= 1'b0;         // write: pass straight to the controller
                            state    <= WRTH;
                        end
                    end
                end

                LOOK: begin
                    if (hit) begin
                        rdata <= hit_word;
                        dtack <= 1'b1;
                        state <= ACK;
                    end else begin
                        mem_cs_n <= 1'b0;             // miss: issue the controller read
                        state    <= FILL;
                    end
                end

                FILL: begin
                    if (!mem_dtack_n) begin
                        data_ram[idx] <= mem_rdata32; // cache the full 32-bit line
                        tag_ram[idx]  <= reqtag;
                        valid[idx]    <= 1'b1;
                        rdata         <= mem_rdata;    // adapter already selected the word
                        mem_cs_n      <= 1'b1;
                        dtack         <= 1'b1;
                        state         <= ACK;
                    end
                end

                WRTH: begin
                    valid[idx] <= 1'b0;               // write-through, no allocate: drop the line
                    if (!mem_dtack_n) begin
                        mem_cs_n <= 1'b1;
                        dtack    <= 1'b1;
                        state    <= ACK;
                    end
                end

                ACK: begin
                    if (cs_n) begin
                        dtack <= 1'b0;
                        state <= IDLE;
                    end
                end

                default: state <= IDLE;
            endcase
        end
    end

endmodule

`default_nettype wire
