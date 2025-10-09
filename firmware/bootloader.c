#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "mackerel.h"
#include "sd.h"
#include "ide.h"
#include "fat16.h"

#define VERSION "0.5.2"

#define INPUT_BUFFER_SIZE 32

void handler_run(uint32_t addr);
void handler_load(uint32_t addr);
void handler_boot();
void handler_zero(uint32_t addr, uint32_t size);
void handler_ide(uint32_t end);
void handler_help();
void handler_info();
uint8_t readline(char *buffer);
void command_not_found(char *command);
void memdump(uint32_t address, uint32_t bytes);
void print_string_bin(char *str, uint8_t max);

void memtest8(uint8_t *start, uint32_t size, uint8_t target);
void memtest16(uint16_t *start, uint32_t size, uint16_t target);
void memtest32(uint32_t *start, uint32_t size);

// Reference RAM info from the linker
extern char __sram_start[];
extern char __sram_length[];
extern char __dram_start[];
extern char __dram_length[];

char buffer[INPUT_BUFFER_SIZE];

struct bi_record
{
    uint16_t tag;    // BI_xxx
    uint16_t size;   // total size of this record in bytes (incl. header)
    uint32_t data[]; // payload
};

struct mem_info
{
    uint32_t addr; // physical base
    uint32_t size; // length in bytes
};

#define BI_LAST 0x0000
#define BI_MACHTYPE 0x0001 /* optional */
#define BI_CPUTYPE 0x0002  /* optional */
#define BI_MMUTYPE 0x0004  /* optional */
#define BI_MEMCHUNK 0x0005
#define BI_RAMDISK 0x0006 /* optional */
#define BI_COMMAND_LINE 0x0007

typedef unsigned long uintptr_t;

static inline uint8_t *align4(uint8_t *p)
{
    uintptr_t v = (uintptr_t)p;
    v = (v + 3) & ~3u;
    return (uint8_t *)v;
}

void emit_bootinfo(uintptr_t _end)
{
    uint8_t *p = align4((uint8_t *)_end);

    // BI_MACHTYPE
    {
        struct bi_record *r = (struct bi_record *)p;
        r->tag = BI_MACHTYPE;
        r->size = sizeof(*r) + sizeof(uint32_t);
        p += sizeof(*r);
        *(uint32_t *)p = 15; // 15
        p += sizeof(uint32_t);
        p = align4(p);
    }

    // BI_MEMCHUNK
    {
        struct bi_record *r = (struct bi_record *)p;
        struct mem_info *mi = (struct mem_info *)(p + sizeof(*r));
        r->tag = BI_MEMCHUNK;
        r->size = sizeof(*r) + sizeof(*mi);
        mi->addr = 0x00000000u;
        mi->size = 0x08000000u; // 128MB
        p += r->size;
        p = align4(p);
    }

    // BI_CPUTYPE
    {
        struct bi_record *r = (struct bi_record *)p;
        r->tag = BI_CPUTYPE;
        r->size = sizeof(*r) + sizeof(uint32_t);
        p += sizeof(*r);
        *(uint32_t *)p = 3; // 3
        p += sizeof(uint32_t);
        p = align4(p);
    }

    // BI_MMUTYPE
    {
        struct bi_record *r = (struct bi_record *)p;
        r->tag = BI_MMUTYPE;
        r->size = sizeof(*r) + sizeof(uint32_t);
        p += sizeof(*r);
        *(uint32_t *)p = 2; // 2
        p += sizeof(uint32_t);
        p = align4(p);
    }

    // BI_COMMAND_LINE (NUL included)
    {
        const char cmd[] = "console=mackerel loglevel=7";
        struct bi_record *r = (struct bi_record *)p;
        const uint32_t paylen = sizeof(cmd);
        r->tag = BI_COMMAND_LINE;
        r->size = sizeof(*r) + paylen;
        p += sizeof(*r);
        memcpy(p, cmd, paylen);
        p += paylen;
        p = align4(p);
    }

    // BI_LAST
    {
        struct bi_record *r = (struct bi_record *)p;
        r->tag = BI_LAST;
        r->size = sizeof(*r); // 4
        // done
    }
}

void handler_help()
{
    printf("Available commands:\r\n");
    printf(" load <addr>           - Load binary from serial into RAM at <addr> (default 0x%X)\r\n", PROGRAM_START);
    printf(" boot                  - Boot Linux from SD\r\n");
    printf(" ide <end>             - Boot Linux from IDE\r\n");
    printf(" run                   - Jump to RAM at 0x%X\r\n", PROGRAM_START);
    printf(" dump <addr>           - Dump 256 bytes of memory starting at <addr>\r\n");
    printf(" peek <addr>           - Peek a byte from memory at <addr>\r\n");
    printf(" poke <addr> <val>     - Poke a byte <val> into memory at <addr>\r\n");
    printf(" mem8 <start> <size>   - Run 8-bit memory test from <start> for <size> bytes\r\n");
    printf(" mem16 <start> <size>  - Run 16-bit memory test from <start> for <size> bytes\r\n");
    printf(" mem32 <start> <size>  - Run 32-bit memory test from <start> for <size> bytes\r\n");
    printf(" zero <start> <size>   - Zero out memory from <start> for <size> bytes\r\n");
    printf(" info                  - Show system information\r\n");
    printf(" help                  - Show this help message\r\n");
}

int main()
{
    duart_puts("\r\n");
    duart_puts("========================================\r\n");
    duart_puts("   " SYSTEM_NAME " Bootloader v" VERSION "\r\n");
    duart_puts("   Build Date: " __DATE__ " - " __TIME__ "\r\n");
    duart_puts("   Copyright (c) 2025 Colin Maykish\r\n");
    duart_puts("   github.com/crmaykish/mackerel-68k\r\n");
    duart_puts("========================================\r\n\r\n");
    duart_puts("Type 'help' for a list of available commands.\r\n\r\n");

    while (true)
    {
        // Present the command prompt and wait for input
        duart_puts("> ");
        readline(buffer);
        duart_puts("\r\n");

        if (strncmp(buffer, "load", 4) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            uint32_t addr = strtoul(param1, 0, 16);
            handler_load(addr);
        }
        else if (strncmp(buffer, "boot", 4) == 0)
        {
            handler_boot();
        }
        else if (strncmp(buffer, "ide", 3) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            uint32_t end = strtoul(param1, 0, 16);
            handler_ide(end);
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
        else if (strncmp(buffer, "info", 4) == 0)
        {
            handler_info();
        }
        else if (strncmp(buffer, "help", 4) == 0)
        {
            handler_help();
        }
        else
        {
            command_not_found(buffer);
        }

        duart_puts("\r\n");
    }

    return 0;
}

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

void handler_boot()
{
    printf("Loading Linux from SD card...\n");

    if (!sd_init())
        return;

    unsigned char first[512];
    unsigned char *mem = (unsigned char *)PROGRAM_START;

    // Read the first block of the SD card to determine the Linux image size
    sd_read(0, first);
    uint32_t image_size = strtoul((char *)first, 0, 10);
    printf("Image size: %lu\n", image_size);

    // Read the rest of the SD card to load the Linux image into RAM
    printf("Loading kernel into 0x%X...\n", (int)mem);

    uint32_t blocks = (image_size / 512) + 1;

    for (int block = 1; block <= blocks; block++)
    {
        if (block % 10 == 0)
        {
            printf("%d/%ld\n", block, blocks);
        }

        sd_read(block, mem);
        mem += 512;
    }

    printf("Done\n");

    handler_run(PROGRAM_START);
}

void block_read(uint32_t block_num, uint8_t *block)
{
    IDE_read_sector((uint16_t *)block, block_num);
}

void handler_ide(uint32_t end)
{
    fat16_boot_sector_t boot_sector;
    fat16_dir_entry_t files_list[16] = {0};

#ifdef MACKEREL_30
    printf("Zeroing memory from 0x%X to 0x%X...\r\n", PROGRAM_START, PROGRAM_START + 0x400000);
    handler_zero(PROGRAM_START, 0x400000);
#endif

    printf("Attempting to load Linux kernel from IDE...\r\n");

    // Reset the IDE interface
    uint16_t buf[256];
    IDE_reset();
    IDE_device_info(buf);

    // Initialize FAT16 library with the IDE block read function
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

                printf("\r\n");

                printf("Read %d of %ld bytes\r\n", bytes_read, files_list[i].file_size);

                if (bytes_read != files_list[i].file_size)
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
        if (end > 0)
        {
            printf("Setting up bootinfo at _end: 0x%lX\r\n", end);
            emit_bootinfo((uintptr_t)end);
        }

        handler_run(PROGRAM_START);
    }
    else
    {
        printf("ERROR: Could not find IMAGE.BIN on disk\r\n");
    }
}

void handler_load(uint32_t addr)
{
    int in_count = 0;
    int end_count = 0;
    uint8_t in = 0;

    if (addr == 0)
    {
        addr = PROGRAM_START;
    }

    printf("Loading from serial into 0x%lX...\r\n", addr);

    while (end_count != 3)
    {
        in = duart_getc();

        MEM(addr + in_count) = in;

        if (in == 0xDE)
        {
            end_count++;
        }
        else
        {
            end_count = 0;
        }

        in_count++;
    }

    MEM(addr + in_count - 3) = 0;

    printf("Done! Transferred %d bytes.\r\n", in_count - 3);
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
    duart_puts("Command not found: ");
    duart_puts(command_name);
}

uint8_t readline(char *buffer)
{
    uint8_t count = 0;
    uint8_t in = duart_getc();

    while (in != '\n' && in != '\r')
    {
        // Character is printable ASCII
        if (in >= 0x20 && in < 0x7F)
        {
            duart_putc(in);

            buffer[count] = in;
            count++;
        }
        else if (0x08)
        {
            // Backspace
            if (count > 0)
            {
                duart_puts("\e[1D"); // Move cursor to the left
                duart_putc(' ');     // Clear last character
                duart_puts("\e[1D"); // Move cursor to the left again
                count--;             // Move input buffer index back
            }
        }

        in = duart_getc();
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
            duart_putc('.');
        }
    }

    printf("\r\nReading...\r\n");
    for (uint32_t *i = start; i < (uint32_t *)(start + size / 4); i++)
    {
        if (*i != (uint32_t)i)
        {
            printf("Error at 0x%lX, expected 0x%lX, got 0x%lX\r\n", (uint32_t)i, (uint32_t)i, *i);
        }

        if ((*i % 0x10000) == 0)
        {
            duart_putc('.');
        }
    }

    printf("\r\nTest complete\r\n");
}
