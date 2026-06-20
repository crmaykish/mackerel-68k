# Mackerel-F timing constraints

# 27 MHz oscillator on the input pin (PLL reference). 1/27 MHz = 37.037 ns
create_clock -name clk_27 -period 37.037 [get_ports {clk_27}]

# 75.6 MHz PLL output. Independent clock (clk_27 only feeds the PLL, so nothing
# crosses the two domains). 1/75.6 MHz = 13.227 ns
create_clock -name clk_soc -period 13.227 [get_nets {clk_soc}]
