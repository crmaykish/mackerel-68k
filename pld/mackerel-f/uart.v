// Mackerel-F UART
// 68k-style wrapper around the Wishbone bus of the OpenCores 16550
// Synchronized to the SoC clock, configured with an 8-bit data bus
`define DATA_BUS_WIDTH_8

module uart (
    input clk,
    input rst_n,

    // 68k bus
    input cs_n,
    input [2:0] reg_addr,  // register select - ADDR_BUS[3:1]
    input rwn,
    input ds_n,
    input [7:0] data_in,
    output [7:0] data_out,
    output dtack_n,
    output irq,

    input rx,
    output tx
);

    // Wishbone bus FSM
    // Start the core cycle and wait for it to ACK before asserting DTACK
    localparam IDLE = 2'd0; // Waiting for access
    localparam REQ = 2'd1; // Strobe asserted, waiting for ACK
    localparam DONE = 2'd2; // ACK received, old DTACK until CPU completes the cycle

    reg [1:0] state = IDLE;
    reg [7:0] rd_data = 8'h00;

    wire wb_ack; // ACK out from the core
    wire [7:0] wb_dat_o; // read data from the core

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
                        rd_data <= wb_dat_o;
                        state <= DONE;
                    end
                DONE:
                    if (cs_n) state <= IDLE;
                default:
                    state <= IDLE;
            endcase
        end
    end

    wire stb = (state == REQ); // Strobe asserted while waiting for ACK
    assign dtack_n = ~(state == DONE); // Assert DTACK when the Wishbone cycle is done
    assign data_out = rd_data;

    // 16550 core
    uart_top core (
        .wb_clk_i(clk),
        .wb_rst_i(~rst_n),

        .wb_adr_i(reg_addr),
        .wb_dat_i(data_in),
        .wb_dat_o(wb_dat_o),
        .wb_we_i(~rwn),
        .wb_stb_i(stb),
        .wb_cyc_i(stb),
        .wb_sel_i(4'b0000),
        .wb_ack_o(wb_ack),

        .int_o(irq),

        .stx_pad_o(tx),
        .srx_pad_i(rx),

        // Modem control is unused
        .cts_pad_i(1'b1),
        .dsr_pad_i(1'b1),
        .ri_pad_i(1'b1),
        .dcd_pad_i(1'b1),
        .rts_pad_o(),
        .dtr_pad_o()
    );

endmodule
