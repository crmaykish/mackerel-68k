# Mackerel 68k

[![Discord](https://img.shields.io/discord/1507804606133043320?logo=discord)](https://discord.gg/DtvXxYCt2Q)

Mackerel 68k is a series of single-board computers based on the Motorola 68000. The goal of this project is to build a computer with each of the major processors in the family, from the 68008 through the 68040. Each iteration will add additional hardware peripherals, but the main software goal is to run Linux on every board.

See the [Hackaday Project Page](https://hackaday.io/project/183861-mackerel-68k-computer) for more pictures, build logs, etc.

## Docs

1. [Mackerel-08 Assembly](docs/mackerel-08-board-assembly.md)
2. [How to Compile Code and Programmable Logic](docs/how-to-compile-everything.md)
3. [Building and Running uClinux (Mackerel-08 and Mackerel-10)](docs/building-and-running-uclinux.md)
4. [Building the Mackerel Toolchains](docs/building-the-mackerel-toolchains.md)
5. [Building and Booting Linux on Mackerel](docs/building-linux-6-for-mackerel-30.md)

## Hardware

### Mackerel-08

Status: Complete

- MC68008 CPU (52-pin PLCC variant)
- 512 KB of Flash ROM, 3.5MB SRAM
- Two serial ports
- SD card support (bitbang SPI)
- Linux 7.1.x NOMMU

![Mackerel-08 SBC v1](media/images/mackerel-08-v1.1_cropped.jpg)

Based on the original prototype hardware, this SBC combines the 52-pin PLCC MC68008, a 512KB Flash ROM, up to 3.5MB of SRAM, and a XR68C681 DUART on a single PCB. The DUART exposes two serial ports and three bit-banged SPI headers. One of these headers is currently used with an SD card breakout board to provide bulk storage.

Three 22V10 PLDs are used for address decoding, interrupt mapping, and DTACK generation. An expansion header breaks out address, data, and control lines to allow additional peripherals to be connected directly to the processor bus.

The address space is mapped as follows:

- RAM:    0x000000 - 0x37FFFF (up to 3.5 MB)
- ROM:    0x380000 - 0x3FBFFF (496/512 KB usable)
- DUART:  0x3FC000 - 0x3FDFFF (8KB)
- Exp:    0x3FE000 - 0x3FFFFF (Expansion header, 8KB)

Mackerel-08 uses a 74HC595 shift register to create a BOOT signal for the first eight /AS cycles of the CPU after reset. This BOOT signal is used by the address decoder PLD to map the ROM to address 0x000000 long enough for the CPU to read the initial stack pointer and program counter vectors from ROM. RAM is mapped to 0x000000 after that.

### Mackerel-10

Status: Complete

- MC68000 or MC68010 CPU
- 1 MB of ROM, 1 MB of SRAM
- Up to 16 MB of 30-pin DRAM (15 usable)
- Two serial ports
- IDE interface
- Linux 7.1.x NOMMU

![Mackerel-10 v1](media/images/mackerel-10-v1.2.jpg)

Mackerel-10 is the second SBC in the project. It expands on the design of Mackerel-08 with a MC68010 CPU (or equivalent), the same XR68C681 DUART, 1MB of Flash ROM, 1MB of SRAM, up to 16 MB of DRAM, and an IDE drive interface. Two CPLDs act as the glue logic and DRAM controller for the board. The SRAM is optional and the address space can be filled almost entirely with DRAM.

The memory map looks like this:

- DRAM:     0x000000 - 0xEFFFFF (15MB)
- ROM:      0xF00000 - 0xFF4000 (not quite 1MB)
- I/O:      0xF40000 - 0xFFFFFF

### Mackerel-30

Status: Prototype

- MC68030 CPU at 20 MHz
- MC68882 FPU
- 512 KB ROM, 512 KB SRAM
- 128 MB of 72-pin DRAM
- Two serial ports
- IDE interface
- Ethernet over SPI
- Linux 7.1.x

![Mackerel-30 v0.1](media/images/mackerel-30-v0.1-assembled.jpg)

Mackerel-30 is based on the 68030 CPU. It includes the DUART and IDE interface from Mackerel-10 and upgrades the DRAM controller to use a 72-pin SIMM. It also includes a MC68882 FPU.

### Mackerel-40
Status: Design

- 68040 CPU at 33 MHz (internal MMU and FPU)
- 2 MB of ROM, 2 MB SRAM
- 2x72 pin SIMMs (up to 256 MB of DRAM)
- Dedicated I/O controller (SPI, I2C, GPIO)
- DUART and IDE from earlier boards

## Software

### Bootloader and Bare-metal Programs
Every version of Mackerel runs a small bootloader program installed on the Flash ROM. This provides a simple set of debugging tools (peek, poke, memtest, etc.) as well as two methods for loading external code.

The bootloader supports the YMODEM protocol for loading programs and data into RAM (see the `ymodem` command), using `lrzsz-sb` on the PC for example. Other terminal programs like TeraTerm should also work.

The `boot` command is used to load the Linux kernel from disk (SD or IDE). It expects a `IMAGE.BIN` file on the first FAT16 partition of the drive. See the `prepare_disk.sh` script in the [mackerel-linux repository](https://github.com/crmaykish/mackerel-linux).

### Linux on Mackerel

All three hardware variants support the latest mainline kernel (v7.1.x). Mackerel-08 and Mackerel-10 run with the NOMMU support and are more limited in their capabilities. Mackerel-30 supports the MMU and the external FPU in Linux and functions more like a traditional headless Linux install. Userspace is made up of busybox (init and shell) with driver support for the DUART, IDE storage, and networking over SPI.

### Compilers and Tools
There are three different toolchains for Mackerel. The baremetal toolchain builds the bootloader and other low level programs in this repo. There are also two variants of the Linux toolchain - one for Mackerel-30 with MMU support and the other for NOMMU/ucLinux on Mackerel-08/10. See [Building the Mackerel Toolchains](docs/building-the-mackerel-toolchains.md) for instructions on building them all with crosstools-ng.
