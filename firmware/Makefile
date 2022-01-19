AS = m68k-linux-gnu-as
LD = m68k-linux-gnu-ld
OBJCOPY = m68k-linux-gnu-objcopy

MEMORY_MAP=linker_mackerel.scr

BINS = nop_loop.bin echo_test.bin ram_test.bin

all: $(BINS)

%.o: %.s
	$(AS) $< -o $@

%.elf: %.o
	$(LD) -T $(MEMORY_MAP) -nostdlib $< -o $@

%.bin: %.elf
	$(OBJCOPY) $< -O binary $@

nop_loop.bin: nop_loop.s

echo_test.bin: echo_test.s

ram_test.bin: ram_test.s

clean:
	rm -f *.o *.elf *.bin