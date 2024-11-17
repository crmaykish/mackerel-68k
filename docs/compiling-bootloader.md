# Compiling The Bootloader and GAL Logic

This document describes building the bootloader and other bare metal programs from source. This process requires a Linux PC, preferably Debian or Ubuntu or one of its derivatives. This will also work in a virtual machine and should work in WSL, but I have not verified this.

## Building the Cross-Compiler

The cross-compiler is made up of three pieces: binutils, gcc, and Baselibc. All of these need to be built locally before they can be used to compile programs for Mackerel.

I've created a [script to automate this process](../tools/build_cross_compiler.sh). If you'd prefer to do this manually or customize any part of it, refer to the script anyway. It will serve as a guide to downloading and building the various pieces of the toolchain.

## Build the Bootloader

Once you have the cross-compiler built and installed, the bootloader and other programs can be compiled by running `make` from the [firmware](../firmware/) folder. By default, this is will create programs for Mackerel-08. To build for Mackerel-10 instead, run `make BOARD=mack10`.

## Running From ROM vs. RAM

The bootloader is compiled to run from ROM. Other bare-metal programs run from RAM. This is set up in the [Makefile](../firmware/Makefile).

To compile a new programs, add an entry in the Makefile.

## Building GAL Logic

On Mackerel-08, three 22V10 GAL chips provide the glue logic for the system. The logic equations for these are defined in the [PLD directory](../pld/mackerel-08/). Compiling them into JED files requires the [galasm tool](https://github.com/daveho/GALasm). There's a `Makefile` in the PLD directory to build all of these JEDs, just run `make`.