# Mackerel 68k SBC

See [Hackaday Project Page](https://hackaday.io/project/183861-mackerel-68k-computer) for more pictures, build logs, etc.

![Mackerel-08 Rev 1](/media/images/mackerel-08-rev0.jpg)

The Mackerel 68k is my series of home-built computers based on the Motorola 68000 family. I am building it from the ground up in phases starting with the baby of the family, the 68008. As my understanding and experience with the system improves, I plan to add additional functionality and support for higher-end CPUs in the 68k line-up.

Here's an outline of the major project goals:

- [x] Build the simplest usable computer with a 68008 CPU, ROM, RAM, and a serial port
- [x] Expand hardware to meet uClinux requirements - timer interrupt, more RAM
- [x] Port uClinux in any form and boot it to an interactive shell
- [ ] Design and manufacture a single-board PCB of the initial 68008 computer
- [ ] Expand the initial design to use a 68020, add persistent storage, networking, possibly DRAM
- [ ] Build a final revision using the 68030 - run full Linux, not just uClinux

## Hardware

The hardware includes the CPU, 512KB of ROM (used by the bootloader), 3.5MB of RAM, and a XR68C681 DUART chip for timer and serial port.

Address decoding and assorted glue logic is done by a handful of ATF16V8B PLD chips.

Programs (including uClinux) can be compiled and copied into RAM over the serial port. The bootloader is responsible for this transaction and for starting the loaded programs.

The first build of this computer uses a custom 80-pin backplane and a combination of PCBs and hand-wired component boards. A single-board PCB version is in the works.

## Software

Mackerel runs a small bootloader from ROM (see firmware directory in this repo) and [uClinux 2.0](https://github.com/crmaykish/mackerel-uclinux-20040218).

The toolchain to build uClinux 2.0 is in this [Github repo](https://github.com/crmaykish/mackerel-m68k-elf-tools-2003). This runs fine for me on Debian 12, but requires enabling the i386 architecture and installing libc:i386 and libgcc:i386 since it's 32-bit toolchain.

The newer (currently unfinished) port of uClinux 4.4 is [here](https://github.com/crmaykish/mackerel-uclinux-20160919). This port is compiling and running, but I have not implemented a serial driver, so the kernel gets as far as launching init and then the output stops.

And my serial transfer tool is [here](https://github.com/crmaykish/ctt). This is used in combination with the bootloader to transfer data (usually program code) into RAM.

## Memory Map

RAM (3.5MB): 0x000000 - 0x380000

ROM (512 KB): 0x380000 - 0x3FC000 (mapped to 0x0000 temporarily at boot)

I/O (16 KB): 0x3FC000 - 0x400000 (DUART and expansion header)
