// 68 bus wrapper for the tiny_spi core
module spi (
    input clk,
    input rst_n,

    // 68k bus (low byte lane)
    input cs_n,
    input [2:0] reg_addr,  // ADDR_L[3:1]
    input rwn,
    input ds_n,            // LDS_n
    input [7:0] data_in,   // D0-7 in
    output [7:0] data_out, // D0-7 out
    output dtack_n,
    output irq,

    // SPI bus
    output mosi,
    output sck,
    input miso,

    output nic_cs_n,
    output nic_irq_en
);

    localparam IDLE = 2'd0, REQ = 2'd1, DONE = 2'd2;

    reg [1:0] state = IDLE;
    reg nic_cs = 1'b0;
    reg [7:0] intc = 8'b0;

    wire wb_ack;
    wire [31:0] wb_dat_o;

    wire spi_sel = ~cs_n & (reg_addr < 3'd5);
    wire cs_sel  = ~cs_n & (reg_addr == 3'd5);
    wire intc_sel = ~cs_n & (reg_addr == 3'd6);
    wire start   = spi_sel & ~ds_n;

    always @(posedge clk) begin
        if (!rst_n) begin
            state <= IDLE;
            nic_cs <= 1'b0;
            intc <= 8'b0;
        end else begin
            case (state)
                IDLE: if (start) state <= REQ;
                REQ:  if (wb_ack) state <= DONE;
                DONE: if (cs_n) state <= IDLE;
                default: state <= IDLE;
            endcase
            if (cs_sel & ~ds_n & ~rwn) nic_cs <= data_in[0];
            if (intc_sel & ~ds_n & ~rwn) intc <= data_in;
        end
    end

    wire stb = (state == REQ);

    assign dtack_n  = ~((state == DONE) | (cs_sel & ~ds_n) | (intc_sel & ~ds_n));
    assign data_out = cs_sel ? {7'b0, nic_cs} : intc_sel  ? intc : wb_dat_o[7:0];
    assign nic_cs_n = ~nic_cs;
    assign nic_irq_en = intc[4];

    tiny_spi #(.BAUD_WIDTH(5), .SPI_MODE(0)) core (
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
