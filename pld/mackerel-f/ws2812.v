// Mackerel-F WS2812B RGB LED - single onboard NeoPixel on the Tang Nano 20k
//
// Register map (byte offsets within the slot; reg_addr = ADDR_BUS[3:1]):
//   0x00  GREEN   W    green colour byte (holding)
//   0x00  STATUS  R    [0] BUSY  1 = a frame is in flight
//   0x02  RED     R/W  red colour byte (holding, readback)
//   0x04  BLUE    R/W  blue colour byte (holding, readback); the write also
//                      latches {G,R,B} and starts one transmit

`default_nettype none

module ws2812 #(
    parameter integer CLK_HZ = 75_600_000 // clk frequency; bit timing derives from this
) (
    input wire clk,
    input wire rst_n,

    input wire cs_n,
    input wire [2:0] reg_addr,
    input wire rwn,
    input wire ds_n,
    input wire [7:0] data_in,
    output reg [7:0] data_out,
    output wire dtack_n,

    output reg led_out
);

    localparam [2:0] REG_G = 3'd0;
    localparam [2:0] REG_R = 3'd1;
    localparam [2:0] REG_B = 3'd2;

    localparam integer T0H_TICKS = (CLK_HZ/1000) * 400 / 1_000_000;  // 0.40us high (bit 0)
    localparam integer T1H_TICKS = (CLK_HZ/1000) * 800 / 1_000_000;  // 0.80us high (bit 1)
    localparam integer BIT_TICKS = (CLK_HZ/1000) * 1250 / 1_000_000; // 1.25us full bit period
    localparam integer RST_TICKS = (CLK_HZ/1000) * 80 / 1_000;       // 80us reset/latch

    localparam integer CW = $clog2(RST_TICKS);

    assign dtack_n = cs_n;
    wire wr = ~cs_n & ~rwn & ~ds_n;

    reg [7:0] g_hold;
    reg [7:0] r_hold;
    reg [7:0] b_hold;

    localparam [1:0] IDLE = 2'd0;
    localparam [1:0] SEND = 2'd1;
    localparam [1:0] RST  = 2'd2;
    reg [1:0] state;
    reg [23:0] shift;
    reg [4:0] bit_idx;
    reg [CW-1:0] cnt;

    wire busy = (state != IDLE);

    always @(*) begin
        case (reg_addr)
            REG_G:   data_out = {7'b0, busy}; // STATUS
            REG_R:   data_out = r_hold;
            REG_B:   data_out = b_hold;
            default: data_out = 8'h00;
        endcase
    end

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state   <= IDLE;
            led_out <= 1'b0;
            shift   <= 24'h0;
            bit_idx <= 5'd0;
            cnt     <= {CW{1'b0}};
            g_hold  <= 8'h00;
            r_hold  <= 8'h00;
            b_hold  <= 8'h00;
        end else begin
            case (state)
                IDLE: led_out <= 1'b0; // line idles low

                SEND: begin
                    led_out <= (cnt < (shift[23] ? T1H_TICKS : T0H_TICKS));
                    if (cnt == BIT_TICKS-1) begin
                        cnt   <= {CW{1'b0}};
                        shift <= {shift[22:0], 1'b0};
                        if (bit_idx == 5'd23) begin
                            bit_idx <= 5'd0;
                            state   <= RST; // all 24 bits sent
                        end else begin
                            bit_idx <= bit_idx + 1'b1;
                        end
                    end else begin
                        cnt <= cnt + 1'b1;
                    end
                end

                RST: begin
                    led_out <= 1'b0; // hold low >50us to latch
                    if (cnt == RST_TICKS-1) begin
                        cnt   <= {CW{1'b0}};
                        state <= IDLE;
                    end else begin
                        cnt <= cnt + 1'b1;
                    end
                end

                default: state <= IDLE;
            endcase

            if (wr && reg_addr == REG_G) g_hold <= data_in;
            if (wr && reg_addr == REG_R) r_hold <= data_in;
            if (wr && reg_addr == REG_B) begin
                b_hold <= data_in;
                if (state == IDLE) begin
                    shift   <= {g_hold, r_hold, data_in}; // GRB, MSB first
                    bit_idx <= 5'd0;
                    cnt     <= {CW{1'b0}};
                    state   <= SEND;
                end
            end
        end
    end

endmodule

`default_nettype wire
