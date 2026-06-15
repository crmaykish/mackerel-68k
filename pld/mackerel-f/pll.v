// pll.v
// Clock generator for Mackerel-F.
// Wraps the Gowin PLL primitive to derive a SoC clock from the onboard 27 MHz oscillator

module pll (
    input  clk_in,     // 27 MHz oscillator
    output clk_out,    // 48 MHz SoC clock
    output locked      // high once the PLL has locked
);

    rPLL rpll (
        .CLKIN(clk_in),
        .CLKOUT(clk_out),
        .LOCK(locked),

        .RESET(1'b0),
        .RESET_P(1'b0),
        .CLKFB(1'b0),        // internal feedback

        .IDSEL(6'b0),        // dynamic-reconfig buses, unused
        .FBDSEL(6'b0),
        .ODSEL(6'b0),
        .PSDA(4'b0),         // dynamic phase / duty / fine-delay, unused
        .DUTYDA(4'b0),
        .FDLY(4'b0),

        .CLKOUTP(),          // phase-shifted output, unused
        .CLKOUTD(),          // divided output, unused
        .CLKOUTD3()          // /3 output, unused
    );

    // The five that matter
    defparam rpll.FCLKIN = "27";
    defparam rpll.IDIV_SEL = 8;           // input  / 9
    defparam rpll.FBDIV_SEL = 15;         // feedback * 16  -> 48 MHz
    defparam rpll.ODIV_SEL = 16;          // VCO = 768 MHz
    defparam rpll.DEVICE = "GW1NR-9C";

    // Tie-offs: static config, single output, internal feedback, no dyn reconfig
    defparam rpll.DYN_IDIV_SEL = "false";
    defparam rpll.DYN_FBDIV_SEL = "false";
    defparam rpll.DYN_ODIV_SEL = "false";
    defparam rpll.DYN_DA_EN = "false";
    defparam rpll.PSDA_SEL = "0000";
    defparam rpll.DUTYDA_SEL = "1000";
    defparam rpll.CLKOUT_FT_DIR = 1'b1;
    defparam rpll.CLKOUTP_FT_DIR = 1'b1;
    defparam rpll.CLKOUT_DLY_STEP = 0;
    defparam rpll.CLKOUTP_DLY_STEP = 0;
    defparam rpll.CLKFB_SEL = "internal";
    defparam rpll.CLKOUT_BYPASS = "false";
    defparam rpll.CLKOUTP_BYPASS = "false";
    defparam rpll.CLKOUTD_BYPASS = "false";
    defparam rpll.DYN_SDIV_SEL = 2;
    defparam rpll.CLKOUTD_SRC = "CLKOUT";
    defparam rpll.CLKOUTD3_SRC = "CLKOUT";

endmodule
