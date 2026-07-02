CORE   = cores/fx68k
UART   = cores/uart16550/rtl/verilog
SDRAM  = cores/sdram-tang-nano-20k/src
SPI    = cores/tiny_spi/rtl/verilog
TOP    = mackerel_f
# QT_GPA_PLATFORM=offscreen is needed to run Gowin in headless mode (e.g. over SSH)
GW       = QT_QPA_PLATFORM=offscreen LD_LIBRARY_PATH=$(HOME)/Gowin_EDA/IDE/lib $(HOME)/Gowin_EDA/IDE/bin/gw_sh
LOADER   = $(HOME)/oss-cad-suite/bin/openFPGALoader
IVERILOG = $(HOME)/oss-cad-suite/bin/iverilog
VVP      = $(HOME)/oss-cad-suite/bin/vvp
BIT      = impl/pnr/$(TOP).fs

# Everything build.tcl feeds to Gowin -> editing any of it forces a re-synth
SRCS   = mackerel_f.v pll.v sdram_controller.v sdram_cache.v $(SDRAM)/sdram.v uart.v spi.v $(SPI)/tiny_spi.v timer.v irq_encoder.v bus_watchdog.v boot_signal.v $(CORE)/fx68k.v $(wildcard $(UART)/*.v)
INPUTS = $(SRCS) $(TOP).cst $(TOP).sdc build.tcl microrom.mem nanorom.mem rom.hex

$(BIT): $(INPUTS)
	$(GW) build.tcl

# Load the bitstream into SPI flash (persistent)
flash: $(BIT)
	$(LOADER) -b tangnano20k -f $(BIT)

# Load the bitstream into SRAM (volatile)
prog: $(BIT)
	$(LOADER) -b tangnano20k $(BIT)


timer-test: timer.v timer_tb.v
	$(IVERILOG) -g2012 -Wall -o timer_tb.vvp timer.v timer_tb.v && $(VVP) timer_tb.vvp

cache-test: sdram_cache.v sdram_cache_tb.v
	$(IVERILOG) -g2012 -o sdram_cache_tb.vvp sdram_cache.v sdram_cache_tb.v && $(VVP) sdram_cache_tb.vvp

clean:
	rm -f $(CORE)/fx68k.v microrom.mem nanorom.mem *.json *.vvp *.vcd
	rm -rf impl

$(CORE)/fx68k.v: $(CORE)/fx68k.sv $(CORE)/fx68kAlu.sv $(CORE)/uaddrPla.sv
	sv2v $^ > $(CORE)/fx68k.v
microrom.mem: $(CORE)/microrom.mem ; cp $< $@
nanorom.mem:  $(CORE)/nanorom.mem ; cp $< $@

.PHONY: flash prog clean test timer-test cache-test
