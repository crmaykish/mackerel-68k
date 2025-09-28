# Building Baremetal Programs (the bootloader), GAL and CPLD Logic

This document describes building the bootloader and other bare metal programs from source. This process requires a Linux PC, ideally a Debian derivative or Arch.

NOTE: Make sure you have the [toolchain](building-the-mackerel-toolchains.md) set up before building code.

## Build the Bootloader

Once you have the cross-compiler built and installed, the bootloader and other programs can be compiled by running `make` from the [firmware](../firmware/) folder. By default, this is will create programs for Mackerel-08. To build for other boards, use the `BOARD` flag with `mack10` or `mack30`, e.g. `make BOARD=mack10`.

## Running From ROM vs. RAM

The bootloader runs from ROM. When compiled, `minipro` can be used with a TL866II-style programmer to flash the resulting `bootloader.bin` file to the ROM chip. On powerup, Mackerel should run it immediately.

On Mackerel-10, the bootloader is split into `-upper.bin` and `-lower.bin` files for the two ROM chips. They should both be flashed each time the bootloader code changes.

All other bare-metal programs are loaded into RAM over serial. The bootloader has a `load` command that is used in combination with [ctt](https://github.com/crmaykish/ctt). Once loaded `run` will start execution.

See the firmware [Makefile](../firmware/Makefile) for all the specifics.

## Building GAL Logic

This only applies to Mackerel-08 which implements all of its programmable logic on three 22V10 GAL chips. The logic equations for these are defined in the [PLD directory](../pld/mackerel-08/). Compiling them into JED files requires the [galasm tool](https://github.com/daveho/GALasm). There's a [Makefile](../pld/mackerel-08/Makefile) in the PLD directory to build all of these JEDs, just run `make`.

They are also flashed using `minipro` and a compatible programmer.

## Building and Flashing the CPLDs

Mackerel-10 and Mackerel-30 both implement glue logic and a DRAM controller on two separate CPLDs. Building and flashing these projects requires Altera Quartus 13.0sp1 and an Altera USB blaster (or clone). The Linux version of this old Quartus application still installs and runs for the most part on modern Linux, but it's a buggy mess and the UI is terrible. I have created Makefiles for each of the CPLD projects. It's much more tolerable to edit the Verilog and pin mappings in a nice editor and then use the Makefiles to compile and flash (`make` and `make flash` respectively) with the command line tools.

Note: JTAG setup can be tricky. You may need to manually run `./altera/13.0sp1/quartus/bin/jtagd` before the USB blaster is detected.
