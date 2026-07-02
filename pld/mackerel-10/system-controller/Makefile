# Path to Quartus installation (override with `make QUARTUS_HOME=/path/to/quartus`)
QUARTUS_HOME ?= ~/altera/13.0sp1/quartus

# Project name (without .qpf/.qsf extensions)
PROJECT := system-controller

# Output directory for compiled files
OUTPUT_DIR := output_files

# Programming file extension (for CPLDs, usually .pof; for FPGAs, .sof)
TARGET_FILE := $(OUTPUT_DIR)/$(PROJECT).pof

# Tools
MAP := $(QUARTUS_HOME)/bin/quartus_map
FIT := $(QUARTUS_HOME)/bin/quartus_fit
ASM := $(QUARTUS_HOME)/bin/quartus_asm
STA := $(QUARTUS_HOME)/bin/quartus_sta
PGM := $(QUARTUS_HOME)/bin/quartus_pgm
JTAGCONFIG := $(QUARTUS_HOME)/bin/jtagconfig

# 68000/68010 CPUs run at half the oscillator frequency
# 68SEC000 on adapter can run at full speed (up to a 33 MHz oscillator)
# Use make SEC=yes to build for 68SEC000, defaults to 68000/68010
SEC ?= no
ifeq ($(SEC),yes)
MAP_ARGS := --verilog_macro=SEC=1
endif

# Default target: build everything
all: $(TARGET_FILE)

# Step 1: Synthesis
$(PROJECT).map:
	$(MAP) $(MAP_ARGS) $(PROJECT)

# Step 2: Fitting
$(PROJECT).fit: $(PROJECT).map
	$(FIT) $(PROJECT)

# Step 3: Assembler (bitstream generation)
$(TARGET_FILE): $(PROJECT).fit
	$(ASM) $(PROJECT)
	$(STA) $(PROJECT)

# Step 4: Flash device
flash: $(TARGET_FILE)
	$(PGM) -m jtag -o "p;$(TARGET_FILE)"

# Step 5: Check connected devices
jtag:
	$(JTAGCONFIG)

# Cleanup
clean:
	rm -f *.rpt *.chg *.eqn *.pin *.sof *.pof *.pof.* *.map *.fit *.asm *.sta
	rm -rf $(OUTPUT_DIR) db incremental_db

.PHONY: all flash jtag clean
