#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "mackerel.h"
#include "sd.h"

#define INPUT_BUFFER_SIZE 32

void handler_runram();
void handler_runrom();
void handler_load(uint32_t addr);
void handler_boot();
void memtest(int start, int end);
uint8_t readline(char *buffer);
void command_not_found(char *command);
void memdump(uint32_t address, uint32_t bytes);
void print_string_bin(char *str, uint8_t max);

char buffer[INPUT_BUFFER_SIZE];

int main()
{
    printf("\r\n### %s Bootloader ###\r\n###   crmaykish - 2024    ###\r\n", SYSTEM_NAME);

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
        else if (strncmp(buffer, "runrom", 6) == 0)
        {
            handler_runrom();
        }
        else if (strncmp(buffer, "run", 3) == 0 || strncmp(buffer, "runram", 6) == 0)
        {
            handler_runram();
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
        else if (strncmp(buffer, "memtest", 7) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            char *param2 = strtok(NULL, " ");
            uint32_t start = strtoul(param1, 0, 16);
            uint32_t stop = strtoul(param2, 0, 16);
            memtest(start, stop);
        }
        else
        {
            command_not_found(buffer);
        }

        duart_puts("\r\n");
    }

    return 0;
}

void handler_runram()
{
    duart_puts("Jumping to 0x400\r\n");
    asm("jsr 0x400");
}

void handler_runrom()
{
    duart_puts("Jumping to 0x100000\r\n");
    asm("jsr 0x100000");
}

void handler_boot()
{
    printf("Loading Linux from SD card...\n");

    bool sd_ready = false;

    for (int i=0;i < 3; i++) {
        sd_ready = sd_init();

        if (sd_ready)
            break;
    }

    if (!sd_ready)
        return;

    unsigned char first[512];
    unsigned char *mem = (unsigned char *)0x400;

    // Read the first block of the SD card to determine the Linux image size
    sd_read(0, first);
    uint32_t image_size = strtoul(first, 0, 10);
    printf("Image size: %u\n", image_size);

    // Read the rest of the SD card to load the Linux image into RAM
    printf("Loading kernel into 0x%X...\n", (int)mem);

    uint32_t blocks = (image_size / 512) + 1;

    for (int block = 1; block <= blocks; block++)
    {
        if (block % 10 == 0) {
            printf("%d/%d\n", block, blocks);
        }
        
        sd_read(block, mem);
        mem += 512;
    }

    printf("Done\n");

    handler_runram();
}

void handler_load(uint32_t addr)
{
    int in_count = 0;
    int end_count = 0;
    uint8_t in = 0;

    if (addr == 0)
    {
        addr = 0x400;
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

    duart_puts("Done!");
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

        in = duart_getc();
    }

    buffer[count] = 0;

    return count;
}

void print_string_bin(char *str, uint8_t max)
{
    uint8_t i = 0;

    while (i < max)
    {
        if (str[i] >= 32 && str[i] < 127)
        {
            duart_putc(str[i]);
        }
        else
        {
            duart_putc('.');
        }

        i++;
    }
}

void memdump(uint32_t address, uint32_t bytes)
{
    uint32_t i = 0;
    uint32_t b = 0;

    printf("%08X  ", address);

    while (i < bytes)
    {
        b = MEM(address + i);
        printf("%02X ", b);

        i++;

        if (i % 16 == 0 && i < bytes)
        {
            printf(" |");
            print_string_bin((char *)(address + i - 16), 16);

            printf("|\r\n%08X  ", address + i);
        }
        else if (i % 8 == 0)
        {
            duart_putc(' ');
        }
    }

    duart_putc('|');
    print_string_bin((char *)(address + i - 16), 16);
    duart_putc('|');
}

void memtest(int start, int end) {
    printf("Starting memory test from 0x%X to 0x%X...\n", start, end);

    for (int i = start; i < end; i++)
    {
        MEM(i) = 0x00;

        if (MEM(i) != 0x00) {
            printf("Memory error at: 0x%X, expected 0x00, actual 0x%X\n", i, MEM(i));
        }

        MEM(i) = 0xAB;

        if (MEM(i) != 0xAB) {
            printf("Memory error at: 0x%X, expected 0xAB, actual 0x%X\n", i, MEM(i));
        }

        MEM(i) = 0xFF;

        if (MEM(i) != 0xFF) {
            printf("Memory error at: 0x%X, expected 0xFF, actual 0x%X\n", i, MEM(i));
        }
    }

    printf("Memory test complete.\n");
}
