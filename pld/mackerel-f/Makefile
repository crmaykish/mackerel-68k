CORE   = cores/fx68k
TOP    = mackerel_f
GW     = LD_LIBRARY_PATH=$(HOME)/Gowin_EDA/IDE/lib $(HOME)/Gowin_EDA/IDE/bin/gw_sh
LOADER = $(HOME)/oss-cad-suite/bin/openFPGALoader
BIT    = impl/pnr/$(TOP).fs

$(BIT): $(CORE)/fx68k.v mackerel_f.v $(TOP).cst microrom.mem nanorom.mem build.tcl
	$(GW) build.tcl

flash: $(BIT)
	$(LOADER) -b tangnano9k -f $(BIT)

clean:
	rm -f $(CORE)/fx68k.v microrom.mem nanorom.mem *.json
	rm -rf impl

$(CORE)/fx68k.v: $(CORE)/fx68k.sv $(CORE)/fx68kAlu.sv $(CORE)/uaddrPla.sv
	sv2v $^ > $(CORE)/fx68k.v
microrom.mem: $(CORE)/microrom.mem ; cp $< $@
nanorom.mem:  $(CORE)/nanorom.mem ; cp $< $@

.PHONY: flash clean
