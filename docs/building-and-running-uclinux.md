## Building and Running uClinux

Mackerel-08 and Mackerel-10 both support uClinux. This document describes the process of building and running uClinux on these boards.

A modern Linux system is required to run all of this. I have verified that it works on Debian 12 and Arch Linux.

### Toolchain Installation

First, make sure you have basic build tools installed on your host system. Install whatever package makes sense for your distro, e.g. build-essential on Debian or base-devel on Arch, or check out the install scripts in [tools](../tools/).

uClinux requires the use of a specific toolchain. Download it from [sourceforge](https://sourceforge.net/projects/uclinux/files/Tools/m68k-uclinux-20160822/m68k-uclinux-tools-20160822.tar.bz2/download).

1. Extract the archive to your home directory: `tar xf m68k-uclinux-tools-20160822.tar.bz2 -C ~`
2. Add the toolchain to your path: `export PATH=$PATH:/home/$(whoami)/usr/local/bin`
3. Verify you can run the compiler: `m68k-uclinux-gcc --version`

If you see the compiler version number printed, you're ready to compile Linux.

### Building Linux

Building uClinux is actually pretty straightforward.

1. Clone the repo: `git clone https://github.com/crmaykish/mackerel-uclinux-20160919.git`
2. cd into the repository
3. Run `make menuconfig`
4. From the Vendor/Product Selection menu, choose Mackerel as the vendor and Mackerel-08 or Mackerel-10 as the product.
5. Exit, saving the configuration.
6. `make`

If everything went well, you will now have an `images/image.bin` file created.

### Booting Linux on Mackerel-08

Mackerel-08 can load Linux from a microSD card through an adapter board. In theory, there are many of these boards that would work, but I have only had good consistency with this [one from Adafruit](https://www.adafruit.com/product/254). The header on Mackerel-08 is designed for this adapter to slot right in. I have tried other cheaper adapters and they all start to fail as clock speeds increase. Since bitbang SPI is already incredibly slow, we can't afford any more bottlenecks.

The bootloader treats the SD card as a big dumb block storage and not like a real drive with partitions and filesystems. There is a script in the root of the uclinux repository called `sd.sh`. Connect a microSD card to your PC through an appropriate adapter and run this script against it. The script will "format" the card in a way that the bootloader can read.

The script is just writing the size of the Linux image to the first few bytes of the block device and then writing the binary data from the Linux image to the drive after that. The bootloader reads the size and then reads as many blocks off the SD card as is necessary to load the Linux image into RAM.

Once you have an SD card created and connected to Mackerel-08, power it on and run `boot` from the console. The bootloader will read the image into RAM and then jump to it to start booting the kernel. This loading procedure can take over 5 minutes depending on the clock speed of the 68008.

Unlike Mackerel-10, Mackerel-08 cannot access the SD card from within uClinux. It's strictly used by the bootloader. Only RAM-backed temporary filesystems as accessible by Linux.

### Booting Linux on Mackerel-10

The Mackerel-10 bootloader expects to find the Linux image file in a FAT16 partition on an IDE drive. There are many IDE devices that should work, including actual hard drives, but I have had the most consistency with CF cards and an IDE converter board.

To prepare the drive, connect it to your host PC with whatever USB adapter makes sense (I use a CF card reader). This drive should show up in `lsblk` as `/dev/sdb` or some other drive letter. Make sure you pick the right one - we're about to wipe it.

From the [tools](../tools/) folder in this repository, run the format disk script: `sudo ./format_disk.sh /dev/sdb`. This script will create two partitions on the drive: a 64 MB FAT16 partition at the start of the drive followed by a 1GB ext2 partition. You can verify this with `sudo fdisk -l /dev/sdb`:

```
Disk /dev/sda: 3.83 GiB, 4110188544 bytes, 8027712 sectors
Disk model: Multi-Card      
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x6b367f2b

Device     Boot  Start     End Sectors  Size Id Type
/dev/sdb1         2048  124927  122880   60M  e W95 FAT16 (LBA)
/dev/sdb2       124928 2078719 1953792  954M 83 Linux
```

All that's left now is to copy the Linux image to the FAT16 partition:

1. `sudo mkdir -p /mnt/cf`
2. `sudo mount /dev/sdb1 /mnt/cf`
3. From the uclinux repo: `sudo cp images/image.bin /mnt/cf/IMAGE.BIN`
4. `sudo umount /dev/sdb1`

Plug the drive back into Mackerel-10, power it on, and run `ide` from the bootloader prompt. Hopefully you are greeted with a short copy sequence followed by kernel boot messages.

Since Mackerel-10 also has an IDE driver in uClinux, you can use that 1GB ext2 partition we created earlier.

From the Mackerel terminal: `fdisk -u -l /dev/hda`

```
Disk /dev/hda: 4110 MB, 4110188544 bytes
16 heads, 63 sectors/track, 7964 cylinders, total 8027712 sectors
Units = sectors of 1 * 512 = 512 bytes

   Device Boot      Start         End      Blocks  Id System
/dev/hda1            2048      124927       61440   e Win95 FAT16 (LBA)
Partition 1 does not end on cylinder boundary
/dev/hda2          124928     2078719      976896  83 Linux
Partition 2 does not end on cylinder boundary
```

You can also mount it and read/write from it:
1. `mkdir /tmp/cf`
2. `mount /dev/hda2 /tmp/cf`
3. `ls -la /tmp/cf`
