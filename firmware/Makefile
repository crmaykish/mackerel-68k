CC = ~/opt/cross/bin/m68k-elf-gcc
AS = ~/opt/cross/bin/m68k-elf-as
LD = ~/opt/cross/bin/m68k-elf-ld
OBJCOPY = ~/opt/cross/bin/m68k-elf-objcopy

MEMORY_MAP_ROM = linker_rom.scr
MEMORY_MAP_RAM = linker_ram.scr

COMMON_OBJS_ROM = vectors.o start.o glue.o mackerel.o
COMMON_OBJS_RAM = start_ram.o glue.o mackerel.o

BINS = bootloader.bin hello.bin kernel.bin

.SECONDARY: bootloader.elf hello.elf

all: $(BINS)

%.o: %.c
	$(CC) -mcpu=68000 -g -Wall -c $< -o $@

%.o: %.s
	$(AS) -mcpu=68000 $< -o $@

%.bin: %.elf
	$(OBJCOPY) $< -O binary $@

bootloader.elf: $(COMMON_OBJS_ROM) bootloader.o ch376s.o
	$(CC) -mcpu=68000 -Wl,--build-id=none -T $(MEMORY_MAP_ROM) -nostartfiles $^ -o $@

hello.elf: $(COMMON_OBJS_RAM) hello.o 
	$(CC) -mcpu=68000 -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@

kernel.elf: $(COMMON_OBJS_RAM) kernel.o
	$(CC) -mcpu=68000 -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@

clean:
	rm -f *.o *.elf *.bin
