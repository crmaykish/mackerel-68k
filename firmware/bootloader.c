#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "mackerel.h"
#include "ch376s.h"

#define INPUT_BUFFER_SIZE 32

uint8_t readline(char *buffer);
void handler_run();
void handler_load();
void handler_boot();
void zero(uint32_t start, uint32_t end);
void command_not_found(char *command);

uint32_t stack_pointer()
{
    uint32_t *stack_pointer;

    asm volatile("move.l %%sp, %0"
                 : "=p"(stack_pointer));

    return (uint32_t)stack_pointer;
}

void print_string_bin(char *str, uint8_t max)
{
    uint8_t i = 0;

    while (i < max)
    {
        if (str[i] >= 32 && str[i] < 127)
        {
            mfp_putc(str[i]);
        }
        else
        {
            mfp_putc('.');
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
            mfp_putc(' ');
        }
    }

    mfp_putc('|');
    print_string_bin((char *)(address + i - 16), 16);
    mfp_putc('|');
}

char buffer[INPUT_BUFFER_SIZE];

int main()
{
    mfp_puts("\r\n### Mackerel-8 Bootloader ###\r\n");

    while (true)
    {
        // Present the command prompt and wait for input
        mfp_puts("> ");
        readline(buffer);
        mfp_puts("\r\n");

        if (strncmp(buffer, "load", 4) == 0)
        {
            handler_load();
        }
        else if (strncmp(buffer, "run", 3) == 0)
        {
            handler_run();
        }
        else if (strncmp(buffer, "boot", 4) == 0)
        {
            handler_boot();
        }
        else if (strncmp(buffer, "zero", 4) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            char *param2 = strtok(NULL, " ");
            uint32_t start = strtoul(param1, 0, 16);
            uint32_t end = strtoul(param2, 0, 16);

            zero(start, end);
        }
        else if (strncmp(buffer, "dump", 4) == 0)
        {
            strtok(buffer, " ");
            char *param1 = strtok(NULL, " ");
            uint32_t addr = strtoul(param1, 0, 16);

            memdump(addr, 256);
        }
        else if (strncmp(buffer, "stack", 5) == 0)
        {
            uint32_t sp = stack_pointer();

            printf("Stack pointer: %X\r\n", sp);
            memdump(sp, MEM_UINT(INIT_SP_ADDRESS) - sp);
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

        mfp_puts("\r\n");
    }

    return 0;
}

void handler_run()
{
    mfp_puts("Jumping to 0x8000\r\n");
    asm("jsr 0x8000");
}

void handler_load()
{
    int in_count = 0;
    int end_count = 0;
    uint8_t in = 0;

    mfp_puts("Loading from serial...\r\n");

    while (end_count != 3)
    {
        in = mfp_getc();

        MEM(0x8000 + in_count) = in;

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

    MEM(0x8000 + in_count - 3) = 0;

    mfp_puts("Done!");
}

// Load a kernel image into RAM, then jump to 0x8000
void handler_boot()
{
    mfp_puts("Copying IMAGE.BIN from USB drive...\r\n");

    if (usb_reset() != CH376S_CMD_RET_SUCCESS)
    {
        mfp_puts("Failed to reset USB module\r\n");
        return;
    }

    size_t kernel_size = file_read("IMAGE.BIN", (uint8_t *)0x8000);

    mfp_puts("\r\n");

    if (kernel_size == 0)
    {
        mfp_puts("Failed to load image file.\r\n");
        return;
    }

    mfp_puts("\r\nKernel image loaded\r\n");

    mfp_puts("Jumping to 0x8000\r\n");

    asm("jsr 0x8000");
}

void zero(uint32_t start, uint32_t end)
{
    if (end <= start)
    {
        printf("Error: end value must be higher than start value");
        return;
    }

    printf("Zeroing RAM from %X to %X... ", start, end);
    memset((void *)start, 0, end - start);
    mfp_puts("Done!");
}

void command_not_found(char *command_name)
{
    mfp_puts("Command not found: ");
    mfp_puts(command_name);
}

uint8_t readline(char *buffer)
{
    uint8_t count = 0;
    uint8_t in = mfp_getc();

    while (in != '\n' && in != '\r')
    {
        // Character is printable ASCII
        if (in >= 0x20 && in < 0x7F)
        {
            mfp_putc(in);

            buffer[count] = in;
            count++;
        }

        in = mfp_getc();
    }

    buffer[count] = 0;

    return count;
}
