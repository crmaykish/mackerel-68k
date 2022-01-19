CC = m68k-linux-gnu-gcc
AS = m68k-linux-gnu-as
LD = m68k-linux-gnu-ld
OBJCOPY = m68k-linux-gnu-objcopy

MEMORY_MAP = linker_mackerel.scr

BINS = monitor.bin

.SECONDARY: monitor.elf

all: $(BINS)

%.o: %.c
	$(CC) -mcpu=68000 -g -Wall -O2 -c $< -o $@

%.o: %.s
	$(AS) -mcpu=68000 $< -o $@

%.bin: %.elf
	$(OBJCOPY) $< -O binary $@

monitor.elf: start.s monitor.c mackerel.c
	$(CC) -mcpu=68000 -Wl,--build-id=none -T $(MEMORY_MAP) -nostdlib -nostartfiles $^ -lgcc -o $@

clean:
	rm -f *.o *.elf *.bin
