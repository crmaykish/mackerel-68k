// Mackerel-F Bus Watchdog
// Trigger a bus error if a bus cycle fails to assert DTACK within TIMEOUT clock cycles

`default_nettype none

module bus_watchdog #(
    parameter integer TIMEOUT = 4096
) (
    input wire clk,
    input wire rst_n,

    input wire as_n,
    input wire dtack_n,
    input wire vpa_n,

    output wire berr_n
);

    localparam integer CW = $clog2(TIMEOUT + 1);

    reg [CW-1:0] count;
    reg berr;

    // If /AS is high or DTACK/VPA is asserted, the current cycle is done
    wire done = as_n | ~dtack_n | ~vpa_n;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            count <= {CW{1'b0}};
            berr <= 1'b0;
        end else if (done) begin
            count <= {CW{1'b0}};
            berr <= 1'b0;
        end else if (count == TIMEOUT[CW-1:0]) begin
            berr <= 1'b1;
        end else begin
            count <= count + 1'b1;
        end
    end

    assign berr_n = ~berr;

endmodule

`default_nettype wire
