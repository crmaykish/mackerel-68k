CC = m68k-elf-gcc
AS = m68k-elf-as
LD = m68k-elf-ld
OBJCOPY = m68k-elf-objcopy

MEMORY_MAP = linker_mackerel.scr

COMMON_OBJS = vectors.o start.o glue.o mackerel.o

BINS = monitor.bin

.SECONDARY: monitor.elf

all: $(BINS)

%.o: %.c
	$(CC) -mcpu=68000 -g -Wall -c $< -o $@

%.o: %.s
	$(AS) -mcpu=68000 $< -o $@

%.bin: %.elf
	$(OBJCOPY) $< -O binary $@

monitor.elf: $(COMMON_OBJS) monitor.o
	$(CC) -mcpu=68000 -Wl,--build-id=none -T $(MEMORY_MAP) -nostartfiles $^ -o $@

clean:
	rm -f *.o *.elf *.bin
