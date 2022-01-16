module mackerel(
    input CLK, RST, A19, AS, DS,
    output ROM_EN, ACIA_EN,
    output reg CLK2
);
    assign ROM_EN = ~(~A19 & ~AS & ~DS);

    assign ACIA_EN = ~(A19 & ~AS & ~DS);

    always @(posedge CLK) begin
        CLK2 <= ~CLK2;
    end

endmodule
