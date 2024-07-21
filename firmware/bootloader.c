#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "mackerel.h"

#define INPUT_BUFFER_SIZE 32

void handler_runram();
void handler_runrom();
void handler_load(uint32_t addr);
void handler_boot();
uint8_t readline(char *buffer);
void command_not_found(char *command);
void memdump(uint32_t address, uint32_t bytes);
void print_string_bin(char *str, uint8_t max);

char buffer[INPUT_BUFFER_SIZE];

int main()
{
    mputs("\r\n### Mackerel-8 Bootloader ###\r\n###   crmaykish - 2024    ###\r\n");

    while (true)
    {
        // Present the command prompt and wait for input
        mputs("> ");
        readline(buffer);
        mputs("\r\n");

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
        else
        {
            command_not_found(buffer);
        }

        mputs("\r\n");
    }

    return 0;
}

void handler_runram()
{
    mputs("Jumping to 0x400\r\n");
    asm("jsr 0x400");
}

void handler_runrom()
{
    mputs("Jumping to 0x300000\r\n");
    asm("jsr 0x300000");
}

void handler_boot()
{
    mputs("Jumping to 0x302000\r\n");
    asm("jsr 0x302000");
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
        in = mgetc();

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

    mputs("Done!");
}

void command_not_found(char *command_name)
{
    mputs("Command not found: ");
    mputs(command_name);
}

uint8_t readline(char *buffer)
{
    uint8_t count = 0;
    uint8_t in = mgetc();

    while (in != '\n' && in != '\r')
    {
        // Character is printable ASCII
        if (in >= 0x20 && in < 0x7F)
        {
            mputc(in);

            buffer[count] = in;
            count++;
        }

        in = mgetc();
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
            mputc(str[i]);
        }
        else
        {
            mputc('.');
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
            mputc(' ');
        }
    }

    mputc('|');
    print_string_bin((char *)(address + i - 16), 16);
    mputc('|');
}