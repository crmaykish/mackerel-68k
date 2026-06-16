// Mackerel-F SPI
// 68k-style wrapper around the OpenCores tiny_spi master core.
// Registers (adr_i, x4 byte stride matching Linux spi-oc-tiny.c):
// 0 RXDATA  1 TXDATA  2 STATUS {-,TXR,TXE}  3 CONTROL  4 BAUD
// CS is a GPIO - tiny_spi has no slave-select output.

module spi (
    input clk,
    input rst_n,

    // 68k bus
    input cs_n,
    input [2:0] reg_addr, // register select - ADDR_BUS[4:2]
    input rwn,
    input ds_n,
    input [7:0] data_in,
    output [7:0] data_out,
    output dtack_n,
    output irq,

    // SPI
    output mosi,
    output sck,
    input miso
);

    // Wishbone bus FSM -- one-clock REQ strobe => one register write per access,
    // so a single TXDATA write starts exactly one byte transfer.
    localparam IDLE = 2'd0; // Waiting for access
    localparam REQ = 2'd1; // Strobe asserted, waiting for ACK
    localparam DONE = 2'd2; // ACK received, hold DTACK until CPU completes

    reg [1:0] state = IDLE;
    reg [7:0] rd_data = 8'h00;

    wire wb_ack; // ACK out from the core (zero wait)
    wire [31:0] wb_dat_o; // read data from the core

    wire start = ~cs_n & ~ds_n;

    always @(posedge clk) begin
        if (!rst_n) begin
            state <= IDLE;
            rd_data <= 8'h00;
        end else begin
            case (state)
                IDLE:
                    if (start) state <= REQ;
                REQ:
                    if (wb_ack) begin
                        rd_data <= wb_dat_o[7:0];
                        state <= DONE;
                    end
                DONE:
                    if (cs_n) state <= IDLE;
                default:
                    state <= IDLE;
            endcase
        end
    end

    wire stb = (state == REQ);
    assign dtack_n = ~(state == DONE);
    assign data_out = rd_data;

    // tiny_spi core, SPI_MODE=0
    tiny_spi #(
        .BAUD_WIDTH(8),
        .SPI_MODE(0)
    ) core (
        .clk_i(clk),
        .rst_i(~rst_n),

        .stb_i(stb),
        .cyc_i(stb),
        .we_i(~rwn),
        .adr_i(reg_addr),
        .dat_i({24'b0, data_in}),
        .dat_o(wb_dat_o),
        .ack_o(wb_ack),
        .int_o(irq),

        .MOSI(mosi),
        .SCLK(sck),
        .MISO(miso)
    );

endmodule
