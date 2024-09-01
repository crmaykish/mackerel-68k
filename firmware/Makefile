CC = ~/opt/cross/bin/m68k-elf-gcc
AS = ~/opt/cross/bin/m68k-elf-as
LD = ~/opt/cross/bin/m68k-elf-ld
OBJCOPY = ~/opt/cross/bin/m68k-elf-objcopy

BOARD = mack08

ifeq ($(BOARD),mack10)
# Mackerel-10
CPPFLAGS = -DMACKEREL_10
MEMORY_MAP_ROM = linker_rom_10.scr
MEMORY_MAP_RAM = linker_ram_10.scr
VECTORS = vectors_10.s
else
# Mackerel-08
MEMORY_MAP_ROM = linker_rom.scr
MEMORY_MAP_RAM = linker_ram.scr
VECTORS = vectors_08.s
endif

COMMON_OBJS_ROM = $(VECTORS) start.o glue.o mackerel.o
COMMON_OBJS_RAM = start_ram.o glue.o mackerel.o

BASE_BINS = bootloader.bin hello.bin kernel.bin

ifeq ($(BOARD),mack10)
# Mackerel-10
BINS = $(BASE_BINS) $(addsuffix -upper.bin, $(basename $(BASE_BINS))) $(addsuffix -lower.bin, $(basename $(BASE_BINS)))
else
# Mackerel-08
BINS = $(BASE_BINS)
endif

.SECONDARY: *.elf *.bin

all: $(BINS)

%.o: %.c
	$(CC) $(CPPFLAGS) -mcpu=68000 -g -Wall -c $< -o $@ -O

%.o: %.s
	$(AS) -mcpu=68000 $< -o $@

%.bin: %.elf
	$(OBJCOPY) $< -O binary $@

bootloader.elf: $(COMMON_OBJS_ROM) bootloader.o sd.o spi.o
	$(CC) $(CPPFLAGS) -mcpu=68000 -Wl,--build-id=none -T $(MEMORY_MAP_ROM) -nostartfiles $^ -o $@

hello.elf: $(COMMON_OBJS_RAM) hello.o 
	$(CC) $(CPPFLAGS) -mcpu=68000 -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@

kernel.elf: $(COMMON_OBJS_RAM) kernel.o
	$(CC) $(CPPFLAGS) -mcpu=68000 -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@
	
%-upper.bin: %.bin
	$(OBJCOPY) --interleave=2 --byte=0 -I binary -O binary $< $@

%-lower.bin: %.bin
	$(OBJCOPY) --interleave=2 --byte=1 -I binary -O binary $< $@

clean:
	rm -f *.o *.elf *.bin
