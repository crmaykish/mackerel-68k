CC = m68k-linux-gnu-gcc
AS = m68k-linux-gnu-as
LD = m68k-linux-gnu-ld
OBJCOPY = m68k-linux-gnu-objcopy

MEMORY_MAP = linker_mackerel.scr

BINS = monitor.bin

.SECONDARY: monitor.elf

all: $(BINS)

%.o: %.c
	$(CC) -mcpu=68000 -g -Wall -c $< -o $@

%.o: %.s
	$(AS) -mcpu=68000 $< -o $@

%.bin: %.elf
	$(OBJCOPY) $< -O binary $@

monitor.elf: start.o monitor.o mackerel.o
	$(CC) -mcpu=68000 -Wl,--build-id=none -T $(MEMORY_MAP) -static -nostartfiles $^ -l:libc.a -o $@

clean:
	rm -f *.o *.elf *.bin
