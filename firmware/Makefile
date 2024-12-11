CC = ~/opt/cross/bin/m68k-elf-gcc
AS = ~/opt/cross/bin/m68k-elf-as
LD = ~/opt/cross/bin/m68k-elf-ld
OBJCOPY = ~/opt/cross/bin/m68k-elf-objcopy

BOARD = mack08

ifeq ($(BOARD),mack30)
CPPFLAGS = -DMACKEREL_30
MEMORY_MAP_ROM = linker_rom_30.scr
MEMORY_MAP_RAM = linker_ram_30.scr
VECTORS = vectors_30.s
CFLAGS = -m68030 -msoft-float -fomit-frame-pointer
AFLAGS = -m68030
else ifeq ($(BOARD),mack10)
# Mackerel-10
CPPFLAGS = -DMACKEREL_10
MEMORY_MAP_ROM = linker_rom_10.scr
MEMORY_MAP_RAM = linker_ram_10.scr
VECTORS = vectors_10.s
CFLAGS = -m68010
AFLAGS = -m68010
else
# Mackerel-08
CPPFLAGS = -DMACKEREL_08
MEMORY_MAP_ROM = linker_rom.scr
MEMORY_MAP_RAM = linker_ram.scr
VECTORS = vectors_08.s
CFLAGS = -m68000
AFLAGS = -m68000
endif

COMMON_OBJS_ROM = $(VECTORS) start.o glue.o mackerel.o term.o
COMMON_OBJS_RAM = start_ram.o glue.o mackerel.o term.o

BASE_BINS = bootloader.bin hello.bin kernel.bin fatfs_demo.bin

ifeq ($(BOARD),mack10)
# Mackerel-10
BINS = $(BASE_BINS) $(addsuffix -upper.bin, $(basename $(BASE_BINS))) $(addsuffix -lower.bin, $(basename $(BASE_BINS)))
else
# Mackerel-08 / Mackerel-30
BINS = $(BASE_BINS)
endif

.SECONDARY: *.elf *.bin

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -g -Wall -c $< -o $@ -O

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.bin: %.elf
	$(OBJCOPY) $< -O binary $@

bootloader.elf: $(COMMON_OBJS_ROM) bootloader.o sd.o spi.o ide.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_ROM) -nostartfiles $^ -o $@

hello.elf: $(COMMON_OBJS_RAM) hello.o ide.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@

fatfs_demo.elf: $(COMMON_OBJS_RAM) fatfs_demo.o fat16.o sd.o spi.o ide.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@

kernel.elf: $(COMMON_OBJS_RAM) kernel.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@
	
%-upper.bin: %.bin
	$(OBJCOPY) --interleave=2 --byte=0 -I binary -O binary $< $@

%-lower.bin: %.bin
	$(OBJCOPY) --interleave=2 --byte=1 -I binary -O binary $< $@

clean:
	rm -f *.o *.elf *.bin
