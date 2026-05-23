# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Mackerel 68k is a series of Motorola 68000-family single-board computers. Three boards are active:
- **Mackerel-08**: 68008 CPU, SRAM only, three 22V10 GAL chips for glue logic, runs uClinux 4.4
- **Mackerel-10**: 68000/68010 CPU, DRAM + SRAM, two Altera CPLDs for glue logic and DRAM controller, runs uClinux 4.4
- **Mackerel-30**: 68030 CPU, 72-pin DRAM SIMM, two Altera CPLDs, MC68882 FPU, runs Linux 6.x with MMU

## Toolchains

Two separate cross-compilers are required, both built via crosstools-ng using the defconfigs in `tools/`:

- **Bare-metal** (`mackerel_crosstools_baremetal_defconfig`): installs to `~/x-tools/m68k-mackerel-elf/bin/`, prefix `m68k-mackerel-elf-`
- **Linux** (`mackerel_crosstools_linux_defconfig`): installs to `~/x-tools/m68k-mackerel-linux-gnu/bin/`, prefix `m68k-mackerel-linux-gnu-`

See `docs/building-the-mackerel-toolchains.md` for build instructions. Run `tools/install_reqs_arch.sh` or `tools/install_reqs_deb.sh` first.

## Building Firmware

All bare-metal programs (bootloader, hello, kernel, fatfs_demo, libctest) are built from `firmware/`:

```sh
# Default: Mackerel-08
make

# Select board with BOARD variable
make BOARD=mack08   # Mackerel-08 (default)
make BOARD=mack10   # Mackerel-10
make BOARD=mack30   # Mackerel-30

make clean
```

Outputs are `.bin` files. On Mackerel-10, the bootloader is additionally split into `-upper.bin` and `-lower.bin` (interleaved bytes) for two ROM chips.

The bootloader (`bootloader.bin`) runs from ROM (flashed with `minipro`). All other programs run from RAM — load them via the bootloader's serial `load` command using the [ctt](https://github.com/crmaykish/ctt) transfer tool, then start with `run`.

## Building PLD/CPLD Logic

**Mackerel-08** (GAL 22V10, requires [galasm](https://github.com/daveho/GALasm)):
```sh
cd pld/mackerel-08
make          # produces .jed files, flash with minipro
```

**Mackerel-10 / Mackerel-30** (Altera CPLD, requires Quartus 13.0sp1):
```sh
cd pld/mackerel-10/dram_controller   # or system-controller, or mackerel-30 variants
make          # compile
make flash    # JTAG flash via USB blaster

# If USB blaster not detected:
./altera/13.0sp1/quartus/bin/jtagd
make jtag     # verify connected devices
```

## Building Linux (Mackerel-30)

Requires the Linux toolchain and a separate kernel repo (branch `mackerel-30-config`):
```sh
export PATH=$PATH:~/x-tools/m68k-mackerel-linux-gnu/bin
make ARCH=m68k mackerel30_defconfig
make ARCH=m68k CROSS_COMPILE=m68k-mackerel-linux-gnu- -j$(nproc)
```

See `docs/building-linux-6-for-mackerel-30.md` and `docs/building-and-running-uclinux.md`.

## Firmware Architecture

### Board Selection

The `BOARD` make variable sets a preprocessor define (`MACKEREL_08`, `MACKEREL_10`, or `MACKEREL_30`) and selects:
- CPU architecture flags (`-m68000`, `-m68010`, `-m68030`)
- Linker script (ROM or RAM variant per board)
- Exception vector file (`vectors_XX.s`)

`firmware/mackerel.h` uses these defines to map DUART, IDE, and `PROGRAM_START` addresses per board. All hardware access goes through the `MEM()`, `MEM16()`, `MEM32()` volatile pointer macros defined there.

### Startup Sequence

- **ROM programs** (bootloader): link with `linker_rom_XX.scr`, start in `start.c`, vectors in `vectors_XX.s`
- **RAM programs**: link with `linker_ram_XX.scr`, start in `start_ram.c`

Both stubs initialize the stack, call newlib init (`newlib_init.c`/`syscalls.c`), then call `main()`.

### Boot Signal (ROM shadowing)

All boards shadow ROM at address 0x0 for the first several bus cycles after reset, so the CPU reads the initial stack pointer and program counter from ROM. After that, RAM is mapped to 0x0. On Mackerel-08 this is done with a 74HC595 shift register; on Mackerel-10/30 it is implemented in the CPLD (`boot_signal.v`).

### Linux Boot on Mackerel-30

The bootloader's `ide` command reads `IMAGE.BIN` from `sda1` (FAT16, 64 MB) into RAM at `PROGRAM_START` (0x1000), then calls `emit_bootinfo()` to write Linux bootinfo records (machine type, memory chunk, CPU type, MMU type, kernel command line) at `_end` — immediately after the kernel image in RAM — before jumping to it.

**Auto-detection of `_end`:** The kernel image embeds a `"MKRL"` magic word (`0x4D4B524C`) followed by the `_end` address in the bootinfo version-number block near the start of `IMAGE.BIN` (placed there by `head.S` under `CONFIG_MACKEREL`). After loading the image, `scan_kernel_end()` scans the first 4 KB for this magic and reads `_end` from the following word. The `ide` command therefore requires no arguments — `_end` is auto-detected from the image itself. A manual override (`ide <end_addr>`) is still accepted as a fallback for older images.

**Disk layout** (written by `makeroot.sh` in the kernel repo):
- `sda1` — FAT16, 64 MB — contains `IMAGE.BIN`
- `sda2` — ext2, 1 GB — persistent Linux root filesystem

The kernel boots into a minimal initramfs trampoline that mounts `sda2` read-write and calls `switch_root` to pivot to the persistent root. The initramfs is embedded in `IMAGE.BIN` via `CONFIG_INITRAMFS_SOURCE`.

### CPLD Logic (Mackerel-30 system_controller.v)

Implements address decoding, wait-state insertion for IDE and DUART, a 100 Hz timer interrupt (24 MHz / 240,000), IRQ priority encoding, DSACK generation, and IACK handling. The FPU (`CS_FPU_n`) is disabled pending future work.
