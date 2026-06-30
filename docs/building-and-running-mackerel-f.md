# Building and Running Mackerel-F (FPGA)

Mackerel-F is an entire Mackerel system - a soft 68000 core plus all of its glue
logic, memory controller, and peripherals - implemented in RTL and synthesized for
a [Tang Nano 20k](https://wiki.sipeed.com/hardware/en/tang/tang-nano-20k/nano-20k.html)
development board. It runs the same bootloader and the same mainline Linux kernel as
the discrete boards.

The design lives in [`pld/mackerel-f/`](../pld/mackerel-f/). The firmware target is
`BOARD=mackf` (see [How to Compile Everything](how-to-compile-everything.md) and the
firmware [Makefile](../firmware/Makefile)).

## Prerequisites

- **Gowin EDA** (the free "Education" edition is fine) for synthesis and place &
  route. The build calls its command-line tool, `gw_sh`.
- **[oss-cad-suite](https://github.com/YosysHQ/oss-cad-suite-build)** for `sv2v`
  (the fx68k core is SystemVerilog and is converted to Verilog before synthesis) and
  `openFPGALoader` (loads the bitstream over the onboard JTAG). Add its `bin/` to your
  `PATH` before running `make`.
- The **bare-metal toolchain** (`m68k-mackerel-elf-`) to build the bootloader ROM
  image. See [Building the Mackerel Toolchains](building-the-mackerel-toolchains.md).

## Fetch the soft cores

The third-party cores (fx68k, the uart16550 and tiny_spi OpenCores blocks, and the
nand2mario SDRAM controller) are pulled in by a script rather than vendored:

```sh
cd pld/mackerel-f
./get_cores.sh        # clones the cores into pld/mackerel-f/cores/
```

## Build the bootloader ROM and the bitstream

The bootloader is baked into the bitstream as on-chip ROM, so it is built first and
copied into the FPGA project, then the whole thing is synthesized.

```sh
# 1. Build the ROM image from the firmware and copy it into the FPGA project
cd firmware
make BOARD=mackf bootloader.hex
cp bootloader.hex ../pld/mackerel-f/rom.hex

# 2. Synthesize, place & route, and load
cd ../pld/mackerel-f
make                  # sv2v + Gowin synth/PnR -> impl/pnr/mackerel_f.fs
make prog             # load into SRAM (volatile, fast to iterate; lost on power cycle)
make flash            # OR: write the SPI flash (persists across a power cycle)
```

Re-run step 1 (and the `cp`) whenever the firmware changes - the Makefile does not
regenerate `rom.hex` automatically, but it does treat `rom.hex` as a synthesis input,
so a changed ROM forces a re-synth on the next `make`.

Other self-contained bare-metal programs can be built the same way and dropped in as
the ROM for bring-up testing, e.g. `make BOARD=mackf sdramtest.hex` for an 8 MB SDRAM
test. The returnable RAM programs (`uarttest`, `spitest`, `sdtest`, ...) are instead
transferred with the bootloader's `ymodem` command and started with `run`.

## Console and reset

The Tang Nano 20k's onboard Sipeed USB bridge presents two serial ports:

- **`/dev/ttyUSB2`** - the Mackerel-F console, **115200 8N1**.
- **`/dev/ttyUSB1`** - JTAG, used by `openFPGALoader` (`make prog` / `make flash`).

There is no reset button (the board's buttons are tied to FPGA configuration straps).
Reset by power-cycling USB, or simply re-load the bitstream - `make prog` reconfigures
the FPGA and restarts the CPU.

## W5500 Ethernet wiring

The microSD is on the onboard slot, but the W5500 NIC is external. It connects to the
Tang Nano 20k header as a SPI device (CS driven by `gpio[7]`):

| Nano 20k pin | Signal | W5500 pin |
|---|---|---|
| 25 | CS   | SCS  |
| 26 | MOSI | MOSI |
| 27 | SCLK | SCLK |
| 28 | MISO | MISO |
| 29 | INT  | INT  |
| 3V3 | power | VCC |
| GND | ground | GND |
| 3V3 | reset (do not float) | RST |

All signals are 3.3 V. Tie the W5500's `RST` to 3.3v.

## Running Linux

Mackerel-F runs the mainline NOMMU kernel as board `f`. Build it from the
[mackerel-linux](https://github.com/crmaykish/mackerel-linux) repo with the same
scripts as the other boards (see [Building and Booting Linux on Mackerel](building-linux-for-mackerel.md)):

```sh
cd mackerel-linux
bash build_busybox.sh f
bash build_rootfs.sh  f
bash build_kernel.sh  f
```

This produces `image.bin` (the kernel, loaded to `0x400`, with the XIP ROMfs root
appended inside it via `MTD_UCLINUX`; the bare ROMfs intermediate is `rom.bin`).
There are three ways to boot it:

1. **From the SD card.** Copy `image.bin` to the FAT16 boot partition (`install_disk.sh f`)
   and use the bootloader's `boot` command - or just power on and let the **five-second
   autoboot** start it.
2. **Over Ethernet (`netboot`).** Run the netboot server from the kernel build tree and
   use the bootloader's `netboot` command. This re-reads the images on every connection,
   so it is the fast edit/build/boot loop:

   ```sh
   cd mackerel-linux
   /path/to/mackerel-68k/tools/netboot_server.py
   ```

   The bootloader assumes the board will use `192.168.1.199` and the server (PC) will be `192.168.1.200`.
   Change these in the [netboot.c](../firmware/netboot.c) file to use different values.
3. **Over serial (`ymodem`).** The slow fallback when there is no SD or network.

A good boot ends at an interactive BusyBox shell on `ttyS0`, with the microSD as
`mmcblk0` and the W5500 bringing up `eth0` (DHCP at boot).
