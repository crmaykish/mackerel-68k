module mackerel(
    input CLK, RST, A19, AS,
    output ROM_EN, ACIA_EN
);

    assign ROM_EN = ~(~A19 & ~AS);

    assign ACIA_EN = ~(A19 & ~AS);

endmodule
