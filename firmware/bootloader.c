#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "mackerel.h"
#include "sd.h"
#include "ide.h"
#include "fat16.h"

#define VERSION "0.4.5"

#define INPUT_BUFFER_SIZE 32

void handler_run(uint32_t addr);
void handler_load(uint32_t addr);
void handler_boot();
void handler_zero(uint32_t addr, uint32_t size);
void handler_ide();
void handler_help();
uint8_t readline(char *buffer);
void command_not_found(char *command);
void memdump(uint32_t address, uint32_t bytes);
void print_string_bin(char *str, uint8_t max);

void memtest8(uint8_t *start, uint32_t size, uint8_t target);
void memtest16(uint16_t *start, uint32_t size, uint16_t target);
void memtest32(uint32_t *start, uint32_t size);

char buffer[INPUT_BUFFER_SIZE];

void handler_help()
{
    printf("Available commands:\r\n");
    printf(" load <addr>           - Load binary from serial into RAM at <addr> (default 0x%X)\r\n", PROGRAM_START);
    printf(" boot                  - Boot Linux from SD\r\n");
    printf(" ide                   - Boot Linux from IDE\r\n");
    printf(" run                   - Jump to RAM at 0x%X\r\n", PROGRAM_START);
    printf(" dump <addr>           - Dump 256 bytes of memory starting at <addr>\r\n");
    printf(" peek <addr>           - Peek a byte from memory at <addr>\r\n");
    printf(" poke <addr> <val>     - Poke a byte <val> into memory at <addr>\r\n");
    printf(" mem8 <start> <size>   - Run 8-bit memory test from <start> for <size> bytes\r\n");
    printf(" mem16 <start> <size>  - Run 16-bit memory test from <start> for <size> bytes\r\n");
    printf(" mem32 <start> <size>  - Run 32-bit memory test from <start> for <size> bytes\r\n");
    printf(" zero <start> <size>   - Zero out memory from <start> for <size> bytes\r\n");
}

int main()
{
    duart_puts("\r\n");
    duart_puts("========================================\r\n");
    duart_puts("   " SYSTEM_NAME " Bootloader\r\n");
    duart_puts("   Version: " VERSION "\r\n");
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
            handler_ide();
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

    printf("Jumping to 0x%X\r\n", addr);

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
    printf("Image size: %u\n", image_size);

    // Read the rest of the SD card to load the Linux image into RAM
    printf("Loading kernel into 0x%X...\n", (int)mem);

    uint32_t blocks = (image_size / 512) + 1;

    for (int block = 1; block <= blocks; block++)
    {
        if (block % 10 == 0)
        {
            printf("%d/%d\n", block, blocks);
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

void handler_ide()
{
    fat16_boot_sector_t boot_sector;
    fat16_dir_entry_t files_list[16] = {0};

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
                printf("\r\nFound IMAGE.BIN, loading it into RAM at 0x%X...\r\n", PROGRAM_START);

                uint8_t *file = (uint8_t *)PROGRAM_START;

                int bytes_read = fat16_read_file(&boot_sector, files_list[i].first_cluster_low, file, files_list[i].file_size);

                printf("\r\n");

                printf("Read %d of %d bytes\r\n", bytes_read, files_list[i].file_size);

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

    printf("Loading from serial into 0x%X...\r\n", addr);

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
    for (uint32_t i = addr; i < addr + size; i++)
    {
        MEM(i) = 0x00;
    }
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
    printf("8-bit Mem Test: %X to %X w/ val %02X\r\n", (uint32_t)start, (uint32_t)(start + size), target);

    for (uint8_t *i = start; i < (uint8_t *)(start + size); i++)
    {
        *i = target;
    }

    for (uint8_t *i = start; i < (uint8_t *)(start + size); i++)
    {
        if (*i != target)
        {
            printf("Error at 0x%X, expected 0x%02X, got 0x%02X\r\n", (uint32_t)i, target, *i);
        }
    }

    printf("\r\n");
}

void memtest16(uint16_t *start, uint32_t size, uint16_t target)
{
    printf("16-bit Mem Test: %X to %X w/ val %04X\r\n", (uint32_t)start, (uint32_t)(start + size / 2), target);

    for (uint16_t *i = start; i < (uint16_t *)(start + size / 2); i++)
    {
        *i = target;
    }

    for (uint16_t *i = start; i < (uint16_t *)(start + size / 2); i++)
    {
        if (*i != target)
        {
            printf("Error at 0x%X, expected 0x%04X, got 0x%04X\r\n", (uint32_t)i, target, *i);
        }
    }

    printf("\r\n");
}

// Write the 32-bit address value to the same address in RAM
void memtest32(uint32_t *start, uint32_t size)
{
    printf("32-bit Mem Test: %X to %X\r\n", (uint32_t)start, (uint32_t)start + (uint32_t)size);

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
            printf("Error at 0x%X, expected 0x%02X, got 0x%02X\r\n", (uint32_t)i, (uint32_t)i, *i);
        }

        if ((*i % 0x10000) == 0)
        {
            duart_putc('.');
        }
    }

    printf("\r\nTest complete\r\n");
}
