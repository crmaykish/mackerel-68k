# Building Mackerel-08

This document describes the process to assemble and bring up Mackerel-08. It is up to date as of the [v1.1 hardware](../releases/mackerel-08/v1.1/) revision.

## Parts

Most of the passives and simpler components are standard values and readily available. Refer to the [BOM](../releases/mackerel-08/v1.1/mackerel-08-v1.1-BOM.csv) for a detailed list.

The 3.3v regulator is an AP1084. There are several alternate part numbers that can be dropped in here as well, just confirm the pinout. This regulator is only required if 3.3v SPI devices are used. In all other cases, it can be omitted without impacting the core system functionality.

### CPU

The Motorola 68008 is available in 8 and 10 MHz speed variants. There's not a huge difference between them and they both overclock to 14+ MHz in my experience. Use whichever version you can find.

### RAM and ROM

Any standard 512 KB ROM chip should work, but I have only personally validated the SST39SF040 in DIP-32 or PLCC ([with adapter](https://github.com/crmaykish/adapters-and-breakout-boards/tree/main/PLCC-32-to-DIP-32)).

The SRAM chips are a little more particular. I've had the best luck with Alliance AS6C4008-55PCN modules in DIP-32 or the Alliance AS7C4096A-15JIN in SOJ-36 ([with adapter](https://github.com/crmaykish/adapters-and-breakout-boards/tree/main/SOJ-36-to-DIP-32-SRAM)).

### Programmable Logic

Three 22V10 GALs are needed to handle the glue logic. These are fairly standard parts and any variant with a 15ns rating or better should work, but I can only vouch for the Atmel/Microchip ATF22V10C-15PU chips.

### DUART

The XR68C681 DUART has recently been marked end-of-life. It's still available at the time of writing (November 2024) on Digikey and some other retailers. I've also had good luck finding them on eBay. Just stick to highly-rated sellers.

### Oscillators

The DUART requires an oscillator running at 3.6864 MHz. Either the half- or full-size can can be installed.

There's a bit more flexibility with the CPU oscillator. Technically, the 68008 can run as slow as 2 MHz and as high as its rated speed, 8 or 10 MHz. I'd recommend using a 10 MHz oscillator in either case. Higher speeds make Linux more usable and all of the CPUs I have tested can handle 14+ MHz without much issue.

I recommend socketing the oscillators so you can experiment.

### SD

Mackerel-08 was designed with this [Adafruit Micro SD breakout](https://www.adafruit.com/product/254) in mind. It slots directly into the SD header on board.

It might be possible to use other SD breakout/level-shifter boards, but a lot of the no-name versions are poor quality or their level shifters are too slow. Try them at your own risk. If you're not using the Adafruit board, you will need to pay attention to the pinouts and make the appropriate adjustments when connecting them to Mackerel. If you do find an alternative that works well, let me know!

## Tools

Besides basic soldering tools, you'll also need some method to program the ROM and the three PLDs. I use and recommend a TL866II Plus programmer. It will handle both the ROM and the PLDs and works with [minipro](https://davidgriffith.gitlab.io/minipro/) software.

## Board Bring-up

### PCB Prep

Once you have all the components and sockets soldered to the board, I recommend giving it a good cleaning with isopropyl alcohol followed by soap and water. Scrub as much of the solder flux residue off the board as you can. Let it dry fully and then populate the sockets.

### Programming The Chips

Download the bootloader ROM and the three JED files from the [release folder](../releases/mackerel-08/v1.1/).

Use `minipro` to flash the bootloader to the ROM chip:

`minipro -p SST39SF040 -s -w bootloader.bin`

Similarly, each of the three PLDs can be programmed:

1. `minipro -p "ATF22V10C(UES)" -w address_decoder.jed`
2. `minipro -p "ATF22V10C(UES)" -w interrupt_decoder.jed`
3. `minipro -p "ATF22V10C(UES)" -w dtack_decoder.jed`

If you have another method for programming ROMs and PLDs, that will almost certainly work as well.

### Booting Up

Connect a serial terminal to UART B. Use 115200 baud, 1 stop bit, no parity bits (likely the default settings).

Apply 12v DC power. You should be greeted with the Mackerel-08 bootloader.

```
### Mackerel-08 Bootloader v0.1 ###
###   crmaykish - 2024    ###
>
```

I'd recommend running a memory test at this point: `memtest 8000 37F000`

If it passes without errors, congrats! Mackerel-08 is operational.
