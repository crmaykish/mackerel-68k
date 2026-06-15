set_device -name GW1NR-9C GW1NR-LV9QN88PC6/I5

add_file cores/fx68k/fx68k.v
add_file mackerel_f.v
add_file pll.v
add_file mackerel_f.cst
add_file mackerel_f.sdc
add_file boot_signal.v

set_option -top_module mackerel_f
set_option -output_base_name mackerel_f

run all
