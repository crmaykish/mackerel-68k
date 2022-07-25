#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "mackerel.h"
#include "ch376s.h"

#define INPUT_BUFFER_SIZE 32

uint8_t readline(char *buffer);
void handler_run();
void handler_load();
void handler_boot();
void handler_zero();
void handler_memtest();
void command_not_found(char *command);

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
            handler_zero();
        }
        else if (strncmp(buffer, "memtest", 6) == 0)
        {
            handler_memtest();
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

// Load a kernel image into RAM at 0x00, then jump to 0x8000
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

void handler_zero()
{
    mfp_puts("Erasing RAM... ");
    memset((void *)0x8000, 0, BOOTLOADER_RAM_START - VECTOR_TABLE_SIZE);
    mfp_puts("Done!");
}

void handler_memtest()
{
    mfp_puts("Starting RAM test...\r\n");

    int start = 0x8000;
    int end = BOOTLOADER_RAM_START - VECTOR_TABLE_SIZE;
    uint8_t val = 0xAA;

    mfp_puts("Setting RAM values to 0xAA...\r\n");

    memset((void *)start, val, end);

    mfp_puts("Checking for write errors... (this will take a while)\r\n");

    for (int i = start; i < end; i++)
    {
        if (MEM(i) != val)
        {
            printf("MEM ERROR AT: %06X, value: %02X\r\n", i, MEM(i));
        }

        MEM(MFP_GPDR) = (i & 0xFF);
    }

    MEM(MFP_GPDR) = 0;

    mfp_puts("Zeroing out RAM...\r\n");
    memset((void *)start, 0, end);

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
