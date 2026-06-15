# Mackerel-F timing constraints

# 27 MHz oscillator on the input pin (PLL reference). 1/27 MHz = 37.037 ns
create_clock -name clk_27 -period 37.037 [get_ports {clk_27}]

# 48 MHz PLL output. Independent clock (clk_27 only feeds the PLL, so nothing
# crosses the two domains). 1/48 MHz = 20.833 ns
create_clock -name clk_soc -period 20.833 [get_nets {clk_soc}]

# fx68k advances only on the enPhi1/enPhi2 enables, 2 clk_soc cycles apart, so
# its internal paths really get 2 cycles. Clock-wide scope
set_multicycle_path 2 -setup -from [get_clocks {clk_soc}] -to [get_clocks {clk_soc}]
set_multicycle_path 1 -hold  -from [get_clocks {clk_soc}] -to [get_clocks {clk_soc}]
