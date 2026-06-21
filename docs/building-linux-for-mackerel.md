# Building and Booting Linux on Mackerel

## Quick reference

```sh
# Replace '30' with '10', '08', or 'f' depending on your target
bash clean_all.sh
bash build_busybox.sh 30
bash build_rootfs.sh 30
bash build_kernel.sh 30
sudo bash prepare_disk.sh /dev/sdX # First time only
sudo bash install_disk.sh /dev/sdX 30
minipro -p SST39SF040 -s -w rom08.bin # Mackerel-08 only
# Insert card, power on, run `boot` from the bootloader
```
---

Every Mackerel board runs the **same mainline Linux kernel** (currently 7.1). There is a single in-tree platform, `arch/m68k/mackerel/`, shared by every board; the per-board differences (CPU, MMU vs. NOMMU, memory map, root filesystem) are selected by a defconfig. Everything is built from the [mackerel-linux](https://github.com/crmaykish/mackerel-linux) repo through the **same handful of scripts**, each of which takes a single board argument:

| Board | Argument | CPU | MMU | Root filesystem | Storage |
|-------|----------|-----|-----|-----------------|---------|
| Mackerel-30 | `30` (default) | 68030 | yes | ext4 on disk (`/dev/sda2`) | IDE / CompactFlash |
| Mackerel-10 | `10` | 68000/68010 | no | embedded initramfs, optional ext root on `/dev/sda2` | IDE / CompactFlash |
| Mackerel-08 | `08` | 68008 | no | ROMfs flashed to the ROM chip | (none) |
| Mackerel-F | `f` | 68000 (fx68k softcore) | no | ROMfs run in place from SDRAM | microSD (SPI) |

Every board boots the same way: the bootloader's `boot` command reads `IMAGE.BIN` from the boot partition of the storage device into RAM and jumps to it. (Mackerel-F can also pull the image over Ethernet with `netboot`; see [Building and Running Mackerel-F (FPGA)](building-and-running-mackerel-f.md).)

In every command below, swap the board argument (`30`, `10`, `08`, or `f`) to target a different board. The argument defaults to `30` if omitted.

## Prerequisites

1. **Toolchains.** Mackerel-30 uses the musl toolchain (`m68k-mackerel-linux-musl-`); Mackerel-08, Mackerel-10, and Mackerel-F share the uClibc NOMMU toolchain (`m68k-mackerel-uclinux-uclibc-`). The build scripts pick the right one from the board argument. See [Building the Mackerel Toolchains](building-the-mackerel-toolchains.md).

2. **The kernel source.**

   ```sh
   git clone https://github.com/crmaykish/mackerel-linux
   cd mackerel-linux
   git checkout mackerel
   ```

3. **Host tools.** `parted` + `mkfs.fat`/`mkfs.ext4` for preparing the storage media. Building the Mackerel-08 root filesystem also needs `genromfs` (on Arch it is in the AUR). `lrzsz` (the `lrzsz-sb` sender) is only needed for the YMODEM fallback described at the end. The `tools/install_reqs_*` scripts in the main repo install these.

## Building

The build is the same three steps for every board - BusyBox, then the root filesystem, then the kernel - run from the repo root:

```sh
bash build_busybox.sh 30    # build BusyBox (bFLT on NOMMU)
bash build_rootfs.sh  30    # assemble the root filesystem for this board
bash build_kernel.sh  30    # distclean + defconfig + build -> image.bin
```

Run them in that order - `build_rootfs.sh` needs the BusyBox binary, and `build_kernel.sh` embeds the root filesystem into the image. Each script prints what it produced; the final artifact is always **`image.bin`** in the repo root.

What each board's scripts produce:

- **Mackerel-30** - BusyBox `busybox` (dynamic musl), a full root tree in `rootfs_mackerel30/` (written to disk separately), and a ~5 MB `image.bin` that loads and runs at `0x1000`.
- **Mackerel-10** - BusyBox `busybox_nommu` (bFLT), an `initramfs.list` that the kernel turns into an *embedded* initramfs, and an `image.bin` (flat binary) that loads and runs at `0x400`.
- **Mackerel-08** - BusyBox `busybox_mackerel08` (bFLT), also assembles `rom08.bin`.
- **Mackerel-F** - BusyBox `busybox_mackerelf` (bFLT), a `romf.bin` execute-in-place ROMfs root, and an `image.bin` (flat binary) that loads and runs at `0x400`.

To start completely fresh, `bash clean_all.sh` removes all build products and intermediate trees.

## Booting

Write `image.bin` to the storage media, put it in the board, and run `boot` from the bootloader prompt. The two disk scripts handle the media for every board and take the same board argument as the build (replace `/dev/sdX` with your card's device - double-check it, this erases the media):

```sh
sudo bash prepare_disk.sh /dev/sdX        # FAT16 16 MB (boot) + ext4 100 MB (root)
sudo bash install_disk.sh /dev/sdX 30     # IMAGE.BIN -> boot partition, rootfs -> root partition
```

`prepare_disk.sh` only needs to be run once per card and should support any drive 128 MB or bigger. After rebuilding the kernel or rootfs, re-run `install_disk.sh` to update the media. What gets written depends on the board:

- **Mackerel-30** - `IMAGE.BIN` to the FAT16 boot partition and the full ext4 root tree to `sda2`. `boot` reads the image into RAM, writes the Linux bootinfo records after it, and jumps to the kernel, which mounts `sda2` as root.
- **Mackerel-10** - `IMAGE.BIN` to the boot partition and the ext root to `sda2`. The image carries a self-contained initramfs, so it boots without a root on `sda2`; if one is present, `/init` `switch_root`s into it (falling back to the RAM initramfs if the disk is absent).
- **Mackerel-08** - just `IMAGE.BIN` to the SD card's FAT partition; the root filesystem is a ROMfs file that gets flashed to the actual ROM chip.
- **Mackerel-F** - `IMAGE.BIN` and `ROMFS.BIN` to the FAT16 boot partition (`install_disk.sh f`); the kernel runs the ROMfs root in place from SDRAM. The image can also be pulled over Ethernet - see [Building and Running Mackerel-F (FPGA)](building-and-running-mackerel-f.md).

> **NOTE:** Mackerel-08 expects its root filesystem to be in the 512KB ROM alongside the bootloader. The `rom08.bin` file can be flashed with minipro any time after running the `build_rootfs.sh` script. This is required to boot Linux.

In every case the board reads the image off its storage device and starts Linux with the `boot` command - no host connection required beyond the serial console.

## Transferring the image over serial (fallback)

Alternatively, the Linux image can be sent to the board over the serial console with YMODEM instead of reading it from storage. This avoids pulling the card, but it is **much slower** (the image is several MB at 115200 baud) and is really just a backup path. From the bootloader prompt:

1. Run `ymodem` to enter receive (default load address: `0x1000` on Mackerel-30, `0x400` on Mackerel-08/10/F).
2. Send `image.bin` with a YMODEM sender, e.g. `lrzsz-sb --ymodem -k -b image.bin`.
3. Run `run` to jump to the kernel.

The repo's `boot08.sh` automates the whole power-cycle → transfer → run → capture loop for Mackerel-08, and the scriptable transfer sequence (holding the serial fd open across `lrzsz-sb`, etc.) is documented in the top-level `CLAUDE.md` under *Automated Hardware-in-the-Loop Workflow*.
