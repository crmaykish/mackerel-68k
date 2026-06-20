CC = m68k-mackerel-elf-gcc
AS = m68k-mackerel-elf-as
LD = m68k-mackerel-elf-ld
OBJCOPY = m68k-mackerel-elf-objcopy

BOARD = mack30

ifeq ($(BOARD),mack30)
# Mackerel-30
SRAM ?= yes
ifeq ($(SRAM),yes)
CPPFLAGS = -DMACKEREL_30 -DMACKEREL_30_SRAM
MEMORY_MAP_ROM = linker_rom_30.scr
VECTORS = vectors_30.s
else
CPPFLAGS = -DMACKEREL_30
MEMORY_MAP_ROM = linker_rom_30_nosram.scr
VECTORS = vectors_30_nosram.s
endif
MEMORY_MAP_RAM = linker_ram_30.scr
CFLAGS = -m68030 -msoft-float -fomit-frame-pointer
AFLAGS = -m68030

else ifeq ($(BOARD),mack10)
# Mackerel-10
SRAM ?= yes
ifeq ($(SRAM),yes)
MEMORY_MAP_ROM = linker_rom_10.scr
else
MEMORY_MAP_ROM = linker_rom_10_nosram.scr
endif
CPPFLAGS = -DMACKEREL_10
MEMORY_MAP_RAM = linker_ram_10.scr
VECTORS = vectors_10.s
CFLAGS = -m68010
AFLAGS = -m68010

else ifeq ($(BOARD),mackf)
# Mackerel-F
CPPFLAGS = -DMACKEREL_F
MEMORY_MAP_ROM = linker_rom_f.scr
MEMORY_MAP_RAM = linker_ram_f.scr
VECTORS = vectors_f.s
CFLAGS = -m68000
AFLAGS = -m68000

else
# Mackerel-08
CPPFLAGS = -DMACKEREL_08
MEMORY_MAP_ROM = linker_rom_08.scr
MEMORY_MAP_RAM = linker_ram_08.scr
VECTORS = vectors_08.s
CFLAGS = -m68000
AFLAGS = -m68000
endif

# UART backend: the DUART boards share one driver; Mackerel-F uses the 16550.
# Add a board with a new UART chip by writing uart_<chip>.c and selecting it here.
ifeq ($(BOARD),mackf)
UART_DRIVER = uart_16550.o
else
UART_DRIVER = uart_xr68c681.o
endif

# Mass-storage drivers linked into the bootloader. Mackerel-F reads SD over the
# tiny_spi master; the DUART boards bitbang SD (mack08) or use IDE (mack10/30).
ifeq ($(BOARD),mackf)
STORAGE_OBJS = sd_spi.o spi_tiny.o fat16.o w5500.o netboot.o
else
STORAGE_OBJS = sd.o spi.o ide.o fat16.o
endif

COMMON_OBJS_ROM = $(VECTORS) start.o newlib_init.o syscalls.o mackerel.o console.o $(UART_DRIVER) term.o
COMMON_OBJS_RAM = start_ram.o newlib_init.o syscalls.o mackerel.o console.o $(UART_DRIVER) term.o

BASE_BINS = bootloader.bin fatfs_demo.bin libctest.bin

ifeq ($(BOARD),mack10)
# Mackerel-10
BINS = $(BASE_BINS) $(addsuffix -upper.bin, $(basename $(BASE_BINS))) $(addsuffix -lower.bin, $(basename $(BASE_BINS)))
else ifeq ($(BOARD),mackf)
# Mackerel-F: bootloader (ROM) + the RAM test apps loaded over ymodem.
BINS = bootloader.bin libctest.bin uarttest.bin irqtest.bin spitest.bin sdtest.bin sdread.bin
else
# Mackerel-08 / Mackerel-30
BINS = $(BASE_BINS)
endif

.SECONDARY: *.elf *.bin

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -g -Wall -c $< -o $@ -O2

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.bin: %.elf
	$(OBJCOPY) $< -O binary $@

# Convert a raw .bin to Verilog readmemh format
%.hex: %.bin
	hexdump -v -e '2/1 "%02x" "\n"' $< > $@

bootloader.elf: $(COMMON_OBJS_ROM) bootloader.o $(STORAGE_OBJS) ymodem.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_ROM) -nostartfiles $^ -o $@ -lc -lgcc

fatfs_demo.elf: $(COMMON_OBJS_RAM) fatfs_demo.o fat16.o sd.o spi.o ide.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@ -lc -lgcc

libctest.elf: $(COMMON_OBJS_RAM) libctest.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@ -lc -lgcc
	
leds.elf: vectors_f.s leds.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_ROM) -nostartfiles $^ -o $@ -lgcc

sdramtest.elf: vectors_f.s sdramtest.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_ROM) -nostartfiles $^ -o $@ -lgcc

# Newlib + console stress test (RAM program, loaded over ymodem then `run`).
uarttest.elf: $(COMMON_OBJS_RAM) uarttest.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@ -lc -lgcc

# Interrupt test: timer (level 6) + UART RX (level 5) (RAM program, ymodem + run).
irqtest.elf: $(COMMON_OBJS_RAM) irqtest.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@ -lc -lgcc

# SPI smoke test: tiny_spi register access + transfer engine (RAM program, ymodem + run).
spitest.elf: $(COMMON_OBJS_RAM) spitest.o spi_tiny.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@ -lc -lgcc

# SD card smoke test: sd_spi init + read CID/CSD/block 0 (RAM program, ymodem + run).
sdtest.elf: $(COMMON_OBJS_RAM) sdtest.o sd_spi.o spi_tiny.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@ -lc -lgcc

# SD file read + throughput test: FAT16 mount + read IMAGE.BIN (RAM program, ymodem + run).
sdread.elf: $(COMMON_OBJS_RAM) sdread.o sd_spi.o spi_tiny.o fat16.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wl,--build-id=none -T $(MEMORY_MAP_RAM) -nostartfiles $^ -o $@ -lc -lgcc

%-upper.bin: %.bin
	$(OBJCOPY) --interleave=2 --byte=0 -I binary -O binary $< $@

%-lower.bin: %.bin
	$(OBJCOPY) --interleave=2 --byte=1 -I binary -O binary $< $@

ifeq ($(BOARD),mack10)
flash: bootloader-upper.bin bootloader-lower.bin
	@echo "Insert ROM UPPER and press Enter..."
	@read _
	minipro -p SST39SF040 -s -w bootloader-upper.bin -y
	@echo "Insert ROM LOWER and press Enter..."
	@read _
	minipro -p SST39SF040 -s -w bootloader-lower.bin -y
else
flash: bootloader.bin
	minipro -p SST39SF040 -s -w bootloader.bin -y
endif

clean:
	rm -f *.o *.elf *.bin *.hex
