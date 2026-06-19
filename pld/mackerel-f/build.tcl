set_device -name GW2AR-18C GW2AR-LV18QN88C8/I7

add_file cores/fx68k/fx68k.v
add_file mackerel_f.v
add_file pll.v

add_file sdram_controller.v
add_file cores/sdram-tang-nano-20k/src/sdram.v

add_file uart.v
add_file cores/uart16550/rtl/verilog/uart_top.v
add_file cores/uart16550/rtl/verilog/uart_wb.v
add_file cores/uart16550/rtl/verilog/uart_regs.v
add_file cores/uart16550/rtl/verilog/uart_receiver.v
add_file cores/uart16550/rtl/verilog/uart_transmitter.v
add_file cores/uart16550/rtl/verilog/uart_rfifo.v
add_file cores/uart16550/rtl/verilog/uart_tfifo.v
add_file cores/uart16550/rtl/verilog/raminfr.v
add_file cores/uart16550/rtl/verilog/uart_sync_flops.v

add_file spi.v
add_file cores/tiny_spi/rtl/verilog/tiny_spi.v

add_file timer.v
add_file irq_encoder.v
add_file bus_watchdog.v

add_file mackerel_f.cst
add_file mackerel_f.sdc
add_file boot_signal.v

set_option -include_path {cores/uart16550/rtl/verilog}
set_option -top_module mackerel_f
set_option -output_base_name mackerel_f

run all
