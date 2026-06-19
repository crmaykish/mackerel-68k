#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "mackerel.h"
#include "console.h"
#include "term.h"
#include "ymodem.h"

#ifdef HAS_DUART_GPIO
#include "uart_xr68c681.h" // the "gpio" command toggles the DUART's port pins
#endif

#include "fat16.h"
#ifdef MACKEREL_F
#include "sd_spi.h" // mackf: SD over the tiny_spi master
#elif defined(MACKEREL_08)
#include "sd.h"     // mack08: bitbang SD
#else
#include "ide.h"    // mack10/mack30: IDE
#endif

#define VERSION "0.8.8"

#define INPUT_BUFFER_SIZE 32

void handler_run(uint32_t addr);
void handler_ymodem(uint32_t addr);
void handler_zero(uint32_t addr, uint32_t size);
void handler_boot();
#ifdef HAS_DUART_GPIO
void handler_gpio(char *dir, char *pin_str, char *val_str);
#endif
void handler_help();
void handler_info();
uint8_t readline(char *buffer);
void command_not_found(char *command);
void memdump(uint32_t address, uint32_t bytes);
void print_string_bin(char *str, uint8_t max);

void memtest8(uint8_t *start, uint32_t size, uint8_t target);
void memtest16(uint16_t *start, uint32_t size, uint16_t target);
void memtest32(uint32_t *start, uint32_t size);

#ifdef MACKEREL_30
void handler_cache(char *sub);
void handler_cinv(char *sub);
void handler_dtest(uint32_t addr, uint32_t size);
#endif

// Reference RAM info from the linker script
extern char __sram_start[];
extern char __sram_length[];
extern char __dram_start[];
extern char __dram_length[];

char buffer[INPUT_BUFFER_SIZE];


void handler_help()
{
    printf("Available commands:\r\n");
    printf(" ymodem <addr>         - Receive a file via YMODEM into RAM at <addr> (default 0x%X)\r\n", PROGRAM_START);
    printf(" boot                  - Load Linux (IMAGE.BIN) from disk and run\r\n");
    printf(" run                   - Jump to RAM at 0x%X\r\n", PROGRAM_START);
    printf(" dump <addr>           - Dump 256 bytes of memory starting at <addr>\r\n");
    printf(" peek <addr>           - Peek a byte from memory at <addr>\r\n");
    printf(" poke <addr> <val>     - Poke a byte <val> into memory at <addr>\r\n");
    printf(" mem8 <start> <size>   - Run 8-bit memory test from <start> for <size> bytes\r\n");
    printf(" mem16 <start> <size>  - Run 16-bit memory test from <start> for <size> bytes\r\n");
    printf(" mem32 <start> <size>  - Run 32-bit memory test from <start> for <size> bytes\r\n");
    printf(" zero <start> <size>   - Zero out memory from <start> for <size> bytes\r\n");
#ifdef HAS_DUART_GPIO
    printf(" gpio in <pin>         - Read DUART input pin IP<pin> (0-6)\r\n");
    printf(" gpio out <pin> <0|1>  - Set DUART output pin OP<pin> (2-7, 0-1 reserved for RTS)\r\n");
#endif
    printf(" info                  - Show system information\r\n");
    printf(" help                  - Show this help message\r\n");
#ifdef MACKEREL_30
    printf(" cache [on|off|i|d]    - Read or set CACR (on=I+D, off=none, i=I-only, d=D-only)\r\n");
    printf(" cinv [i|d]            - Invalidate cache entries (default: both)\r\n");
    printf(" dtest [addr] [size]   - DRAM stress test: write/random-read/linear-verify\r\n");
#endif
}

int main()
{
    // Re-enable the cursor in case a reset interrupted a transfer that had
    // hidden it while drawing the progress bar (see fat16/sd read loops).
    term_cursor_set_vis(true);

    console_puts("\r\n");
    console_puts("========================================\r\n");
    console_puts("   " SYSTEM_NAME " Bootloader v" VERSION "\r\n");
    console_puts("   Build Date: " __DATE__ " - " __TIME__ "\r\n");
    console_puts("   Copyright (c) 2026 Colin Maykish\r\n");
    console_puts("   github.com/crmaykish/mackerel-68k\r\n");
    console_puts("========================================\r\n\r\n");
    console_puts("Type 'help' for a list of available commands.\r\n\r\n");

    while (true)
    {
        // Present the command prompt and wait for input
        console_puts("> ");
        readline(buffer);
        console_puts("\r\n");

        if (strncmp(buffer, "ymodem", 6) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            uint32_t addr = strtoul(param1, 0, 16);
            handler_ymodem(addr);
        }
        else if (strncmp(buffer, "boot", 4) == 0)
        {
            handler_boot();
        }
        else if (strncmp(buffer, "run", 3) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            uint32_t addr = strtoul(param1, 0, 16);
            handler_run(addr);
        }
        else if (strncmp(buffer, "dump", 4) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            uint32_t addr = strtoul(param1, 0, 16);

            memdump(addr, 256);
        }
        else if (strncmp(buffer, "peek", 4) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            uint32_t addr = strtoul(param1, 0, 16);

            printf("%02X", MEM(addr));
        }
        else if (strncmp(buffer, "poke", 4) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            char *param2 = strtok(NULL, " ");
            uint32_t addr = strtoul(param1, 0, 16);
            uint8_t val = (uint8_t)strtoul(param2, 0, 16);

            MEM(addr) = val;
        }
        else if (strncmp(buffer, "mem8", 4) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            char *param2 = strtok(NULL, " ");
            uint32_t start = strtoul(param1, 0, 16);
            uint32_t size = strtoul(param2, 0, 16);

            memtest8((uint8_t *)start, size, 0x00);
            memtest8((uint8_t *)start, size, 0xAA);
            memtest8((uint8_t *)start, size, 0x55);
            memtest8((uint8_t *)start, size, 0xFF);

            printf("Test complete\r\n");
        }
        else if (strncmp(buffer, "mem16", 4) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            char *param2 = strtok(NULL, " ");
            uint32_t start = strtoul(param1, 0, 16);
            uint32_t size = strtoul(param2, 0, 16);

            memtest16((uint16_t *)start, size, 0x0000);
            memtest16((uint16_t *)start, size, 0xAABB);
            memtest16((uint16_t *)start, size, 0x55CC);
            memtest16((uint16_t *)start, size, 0xFFFF);

            printf("Test complete\r\n");
        }
        else if (strncmp(buffer, "mem32", 5) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            char *param2 = strtok(NULL, " ");
            uint32_t start = strtoul(param1, 0, 16);
            uint32_t size = strtoul(param2, 0, 16);
            memtest32((uint32_t *)start, size);
        }
        else if (strncmp(buffer, "zero", 4) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            char *param2 = strtok(NULL, " ");
            uint32_t start = strtoul(param1, 0, 16);
            uint32_t size = strtoul(param2, 0, 16);
            handler_zero(start, size);
        }
#ifdef HAS_DUART_GPIO
        else if (strncmp(buffer, "gpio", 4) == 0)
        {
            strtok(buffer, " ");
            char *dir     = strtok(NULL, " ");
            char *pin_str = strtok(NULL, " ");
            char *val_str = strtok(NULL, " ");
            handler_gpio(dir, pin_str, val_str);
        }
#endif
        else if (strncmp(buffer, "info", 4) == 0)
        {
            handler_info();
        }
        else if (strncmp(buffer, "help", 4) == 0)
        {
            handler_help();
        }
#ifdef MACKEREL_30
        else if (strncmp(buffer, "cache", 5) == 0)
        {
            strtok(buffer, " ");
            char *sub = strtok(NULL, " ");
            handler_cache(sub);
        }
        else if (strncmp(buffer, "cinv", 4) == 0)
        {
            strtok(buffer, " ");
            char *sub = strtok(NULL, " ");
            handler_cinv(sub);
        }
        else if (strncmp(buffer, "dtest", 5) == 0)
        {
            strtok(buffer, " ");
            char *p1 = strtok(NULL, " ");
            char *p2 = strtok(NULL, " ");
            uint32_t addr = p1 ? strtoul(p1, 0, 16) : 0;
            uint32_t size = p2 ? strtoul(p2, 0, 16) : 0;
            handler_dtest(addr, size);
        }
#endif
        else
        {
            command_not_found(buffer);
        }

        console_puts("\r\n");
    }

    return 0;
}

#ifdef HAS_DUART_GPIO
void handler_gpio(char *dir, char *pin_str, char *val_str)
{
    if (!dir || !pin_str)
    {
        printf("Usage: gpio <in|out> <pin> [0|1]\r\n");
        return;
    }

    uint8_t pin = (uint8_t)strtoul(pin_str, 0, 10);

    if (strncmp(dir, "in", 2) == 0)
    {
        if (pin > 6)
        {
            printf("Input pins are IP0-IP6\r\n");
            return;
        }
        uint8_t ipr = MEM(DUART1_IP);
        printf("IP%d = %d\r\n", pin, (ipr >> pin) & 1);
    }
    else if (strncmp(dir, "out", 3) == 0)
    {
        if (pin > 7)
        {
            printf("Output pins are OP2-OP7\r\n");
            return;
        }
        if (pin < 2)
        {
            printf("OP0-OP1 reserved for RTS\r\n");
            return;
        }
        if (!val_str)
        {
            printf("Usage: gpio out <pin> <0|1>\r\n");
            return;
        }
        uint8_t val = (uint8_t)strtoul(val_str, 0, 10);
        if (val == 1)
        {
            MEM(DUART1_OPR_RESET) = (1 << pin);
            printf("OP%d = 1\r\n", pin);
        }
        else if (val == 0)
        {
            MEM(DUART1_OPR) = (1 << pin);
            printf("OP%d = 0\r\n", pin);
        }
        else
        {
            printf("Value must be 0 or 1\r\n");
        }
    }
    else
    {
        printf("Direction must be 'in' or 'out'\r\n");
    }
}
#endif /* HAS_DUART_GPIO */

void handler_run(uint32_t addr)
{
    if (addr == 0)
    {
        addr = PROGRAM_START;
    }

    printf("Jumping to 0x%lX\r\n", addr);

    // Jump to the subroutine at the specified address
    // Programs will return control to the bootloader when they exit because jsr is used
    __asm__ volatile(
        "move.l %0, %%a0\n\t"
        "jsr (%%a0)"
        :
        : "r"(addr)
        : "a0");
}

// Define the block_read function based on the board.
// Mackerel-08 uses bitbang SD. Mackerel-10 and Mackerel-30 have real IDE
#ifdef MACKEREL_F
static int block_read(uint32_t block_num, uint8_t *block, uint32_t count)
{
    return sd_spi_read(block_num, block, count);
}
#elif defined(MACKEREL_08)
static int block_read(uint32_t block_num, uint8_t *block, uint32_t count)
{
    return sd_read_blocks(block_num, count, block) ? 0 : -1;
}
#else
static int block_read(uint32_t block_num, uint8_t *block, uint32_t count)
{
    return IDE_read_sectors((uint16_t *)block, block_num, (uint8_t)count);
}
#endif

void handler_boot()
{
    fat16_boot_sector_t boot_sector;
    fat16_dir_entry_t files_list[16] = {0};

#ifdef MACKEREL_F
    printf("Loading IMAGE.BIN from SD card...\r\n");

    if (!sd_spi_init())
    {
        printf("SD init failed\r\n");
        return;
    }
    sd_spi_print_info();
#elif defined(MACKEREL_08)
    printf("Loading Linux kernel from SD card...\r\n");

    if (!sd_init())
        return;
#else
    printf("Loading Linux kernel from IDE...\r\n");

    // Reset the IDE interface
    uint16_t buf[256];
    IDE_reset();
    IDE_device_info(buf);
#endif

    // Initialize FAT16 library with the board's block read function
    if (fat16_init(block_read) != 0)
    {
        printf("Failed to initialize FAT16 library\r\n");
        return;
    }

    fat16_read_boot_sector(2048, &boot_sector);
    fat16_print_boot_sector_info(&boot_sector);

    printf("\r\nReading files on disk...\r\n");
    fat16_list_files(&boot_sector, files_list);

    bool kernel_found = false;

    for (int i = 0; i < 16; i++)
    {
        if (files_list[i].file_size > 0)
        {
            char filename[13];
            fat16_get_file_name(&files_list[i], filename);

            if (strncmp(filename, "IMAGE   .BIN", 12) == 0)
            {
                printf("\r\nReading IMAGE.BIN (%ld bytes) into RAM at 0x%X...\r\n", files_list[i].file_size, PROGRAM_START);

                uint8_t *file = (uint8_t *)PROGRAM_START;

                int bytes_read = fat16_read_file(&boot_sector, files_list[i].first_cluster_low, file, files_list[i].file_size);

                printf("Read %d of %ld bytes\r\n", bytes_read, files_list[i].file_size);

                if (bytes_read != (int)files_list[i].file_size)
                {
                    printf("File read failed\r\n");
                }
                else
                {
                    printf("File read successfully\r\n");
                    kernel_found = true;
                }

                break;
            }
        }
    }

    if (kernel_found)
    {
        handler_run(PROGRAM_START);
    }
    else
    {
        printf("ERROR: Could not find IMAGE.BIN on disk\r\n");
    }
}

void handler_ymodem(uint32_t addr)
{
    static char name[128];
    uint32_t size = 0;

    if (addr == 0) {
        addr = PROGRAM_START;
    }

    // TODO properly calculate max buffer size based on available RAM
#ifdef MACKEREL_08
    uint32_t bufsz = 0x300000; // 3MB
#else
    uint32_t bufsz = 0x800000; // 8 MB
#endif

    printf("Ready to receive at 0x%lx over YMODEM...\r\n", (unsigned long)addr);

    long n = ymodem_recv((uint8_t *)addr, bufsz, name, &size);

    if (n < 0)
    {
        printf("\r\nYMODEM transfer failed (%ld).\r\n", n);
        return;
    }

    printf("\r\nReceived '%s' (%ld bytes) into 0x%lx.\r\n", name, n, (unsigned long)addr);
    
    if (size && (uint32_t)n != size)
    {
        printf("Warning: stored %ld of %lu bytes (buffer limit?).\r\n", n, (unsigned long)size);
    }
}

void handler_zero(uint32_t addr, uint32_t size)
{
    uint32_t *p = (uint32_t *)addr;
    uint32_t n = size / 4;

    // Unroll 8 stores per loop for speed
    while (n >= 8)
    {
        p[0] = 0;
        p[1] = 0;
        p[2] = 0;
        p[3] = 0;
        p[4] = 0;
        p[5] = 0;
        p[6] = 0;
        p[7] = 0;
        p += 8;
        n -= 8;
    }
    while (n--)
    {
        *p++ = 0;
    }

    // Handle leftover bytes
    addr += (size & ~3u);
    for (uint32_t i = 0; i < (size & 3u); i++)
        MEM(addr + i) = 0;
}

void handler_info()
{
    printf("System Information:\r\n");
    printf(" System: " SYSTEM_NAME "\r\n");
    printf(" SRAM: 0x%08lX to 0x%08lX (%ld KB)\r\n", (uint32_t)__sram_start, (uint32_t)(__sram_start + (uint32_t)__sram_length), (uint32_t)__sram_length / 1024);
    printf(" DRAM: 0x%08lX to 0x%08lX (%ld KB)\r\n", (uint32_t)__dram_start, (uint32_t)(__dram_start + (uint32_t)__dram_length), (uint32_t)__dram_length / 1024);
}

void command_not_found(char *command_name)
{
    console_puts("Command not found: ");
    console_puts(command_name);
}

uint8_t readline(char *buffer)
{
    uint8_t count = 0;
    uint8_t in = console_getc();

    while (in != '\n' && in != '\r')
    {
        // Character is printable ASCII
        if (in >= 0x20 && in < 0x7F)
        {
            console_putc(in);

            buffer[count] = in;
            count++;
        }
        // Backspace
        else if (in == 0x08 || in == 0x7F)
        {
            if (count > 0)
            {
                console_puts("\e[1D"); // Move cursor to the left
                console_putc(' ');     // Clear last character
                console_puts("\e[1D"); // Move cursor to the left again
                count--;             // Move input buffer index back
            }
        }

        in = console_getc();
    }

    buffer[count] = 0;

    return count;
}

void memtest8(uint8_t *start, uint32_t size, uint8_t target)
{
    printf("8-bit Mem Test: %lX to %lX w/ val %02X\r\n", (uint32_t)start, (uint32_t)(start + size), target);

    for (uint8_t *i = start; i < (uint8_t *)(start + size); i++)
    {
        *i = target;
    }

    for (uint8_t *i = start; i < (uint8_t *)(start + size); i++)
    {
        if (*i != target)
        {
            printf("Error at 0x%lX, expected 0x%02X, got 0x%02X\r\n", (uint32_t)i, target, *i);
        }
    }

    printf("\r\n");
}

void memtest16(uint16_t *start, uint32_t size, uint16_t target)
{
    printf("16-bit Mem Test: %lX to %lX w/ val %04X\r\n", (uint32_t)start, (uint32_t)(start + size / 2), target);

    for (uint16_t *i = start; i < (uint16_t *)(start + size / 2); i++)
    {
        *i = target;
    }

    for (uint16_t *i = start; i < (uint16_t *)(start + size / 2); i++)
    {
        if (*i != target)
        {
            printf("Error at 0x%lX, expected 0x%04X, got 0x%04X\r\n", (uint32_t)i, target, *i);
        }
    }

    printf("\r\n");
}

#ifdef MACKEREL_30

static uint32_t get_cacr(void)
{
    uint32_t v;
    asm volatile("movec %%cacr, %0" : "=d"(v));
    return v;
}

static void set_cacr(uint32_t v)
{
    asm volatile("movec %0, %%cacr" : : "d"(v));
}

void handler_cache(char *sub)
{
    if (!sub || sub[0] == '\0')
    {
        uint32_t cacr = get_cacr();
        printf("CACR = 0x%04lX\r\n", cacr);
        printf("I-cache: %s%s\r\n",
               (cacr & 0x0001) ? "enabled" : "disabled",
               (cacr & 0x0002) ? " (frozen)" : "");
        printf("D-cache: %s%s\r\n",
               (cacr & 0x0100) ? "enabled" : "disabled",
               (cacr & 0x0200) ? " (frozen)" : "");
    }
    else if (strncmp(sub, "on", 2) == 0)
    {
        set_cacr(0x0909);
        printf("I+D caches enabled and cleared\r\n");
    }
    else if (strncmp(sub, "off", 3) == 0)
    {
        set_cacr(0x0000);
        printf("Caches disabled\r\n");
    }
    else if (strncmp(sub, "i", 1) == 0)
    {
        set_cacr(0x0009);
        printf("I-cache enabled and cleared\r\n");
    }
    else if (strncmp(sub, "d", 1) == 0)
    {
        set_cacr(0x0900);
        printf("D-cache enabled and cleared\r\n");
    }
    else
    {
        printf("Usage: cache [on|off|i|d]\r\n");
    }
}

void handler_cinv(char *sub)
{
    uint32_t mask;
    if (!sub || sub[0] == '\0')
        mask = 0x0808;
    else if (strncmp(sub, "i", 1) == 0)
        mask = 0x0008;
    else if (strncmp(sub, "d", 1) == 0)
        mask = 0x0800;
    else
    {
        printf("Usage: cinv [i|d]\r\n");
        return;
    }
    set_cacr(get_cacr() | mask);
    printf("Cache invalidated\r\n");
}

void handler_dtest(uint32_t addr, uint32_t size)
{
    if (addr == 0) addr = 0x1000;
    if (size == 0) size = 0x200000;
    addr &= ~0x3u;
    size &= ~0x3u;

    volatile uint32_t *p = (volatile uint32_t *)addr;
    uint32_t n = size / 4;

    printf("dtest: addr=0x%lX size=0x%lX (%ld slots)\r\n", addr, size, n);
    printf("Writing address pattern...\r\n");

    set_cacr(0x0000);
    for (uint32_t i = 0; i < n; i++)
        p[i] = addr + i * 4;

    printf("Random reads...\r\n");
    set_cacr(0x0909);

    uint32_t seed = 0;
    uint32_t rand_fail = 0;
    for (int k = 0; k < 1024; k++)
    {
        seed = seed * 1664525u + 1013904223u;
        uint32_t idx = seed % n;
        uint32_t expected = addr + idx * 4;
        uint32_t got = p[idx];
        if (got != expected)
        {
            if (rand_fail < 5)
                printf("FAIL [%d] addr=0x%lX expected=0x%lX got=0x%lX\r\n",
                       k, addr + idx * 4, expected, got);
            rand_fail++;
        }
    }

    printf("Linear scan...\r\n");
    set_cacr(0x0000);
    uint32_t linear_fail = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        uint32_t expected = addr + i * 4;
        uint32_t got = p[i];
        if (got != expected)
        {
            if (linear_fail < 5)
                printf("FAIL at 0x%lX: expected 0x%lX, got 0x%lX\r\n",
                       addr + i * 4, expected, got);
            linear_fail++;
        }
    }

    if (rand_fail == 0 && linear_fail == 0)
        printf("PASS (1024 random, %ld linear)\r\n", n);
    else
        printf("FAIL: %ld random errors, %ld linear errors\r\n", rand_fail, linear_fail);
}

#endif /* MACKEREL_30 */

// Write the 32-bit address value to the same address in RAM
void memtest32(uint32_t *start, uint32_t size)
{
    printf("32-bit Mem Test: %lX to %lX\r\n", (uint32_t)start, (uint32_t)start + (uint32_t)size);

    printf("Writing...\r\n");
    for (uint32_t *i = start; i < (uint32_t *)(start + size / 4); i++)
    {
        *i = (uint32_t)i;

        if ((*i % 0x10000) == 0)
        {
            console_putc('.');
        }
    }

    printf("\r\nReading...\r\n");
    for (uint32_t *i = start; i < (uint32_t *)(start + size / 4); i++)
    {
        uint32_t got = *i;
        if (got != (uint32_t)i)
        {
            printf("Error at 0x%lX, expected 0x%lX, got 0x%lX\r\n", (uint32_t)i, (uint32_t)i, got);
        }

        if ((got % 0x10000) == 0)
        {
            console_putc('.');
        }
    }

    printf("\r\nTest complete\r\n");
}
