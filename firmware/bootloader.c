
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
void handler_print();
void command_not_found(char *command);

char buffer[INPUT_BUFFER_SIZE];

int main()
{
    int a = 0;
    while (true)
    {
        MEM(MFP_GPDR) = a;
        a += 1;

        delay(1000);
    }

    // serial_puts("\r\n### Mackerel-8 Bootloader ###\r\n");

    // while (true)
    // {
    //     // Present the command prompt and wait for input
    //     serial_puts("> ");
    //     readline(buffer);
    //     serial_puts("\r\n");

    //     if (strncmp(buffer, "load", 4) == 0)
    //     {
    //         handler_load();
    //     }
    //     else if (strncmp(buffer, "run", 3) == 0)
    //     {
    //         handler_run();
    //     }
    //     else if (strncmp(buffer, "print", 5) == 0)
    //     {
    //         handler_print();
    //     }
    //     else if (strncmp(buffer, "boot", 4) == 0)
    //     {
    //         handler_boot();
    //     }
    //     else
    //     {
    //         command_not_found(buffer);
    //     }

    //     serial_puts("\r\n");
    // }

    return 0;
}

void handler_print()
{
    serial_puts((char *)0x80000);
}

void handler_run()
{
    serial_puts("Run loaded program\r\n");
    asm("jsr 0x80000");
}

void handler_load()
{
    int in_count = 0;
    int magic_count = 0;
    uint8_t in = 0;

    MEM(MFP_GPDR) = 0x01;

    serial_puts("Loading from serial...\r\n");

    while (magic_count != 3)
    {
        in = serial_getc();

        MEM(0x80000 + in_count) = in;

        if (in == 0xDE)
        {
            magic_count++;
        }
        else
        {
            magic_count = 0;
        }

        in_count++;
    }

    MEM(0x80000 + in_count - 3) = 0;

    MEM(MFP_GPDR) = 0xF0;

    serial_puts("Done!");
}

void handler_boot()
{
    serial_puts("Booting from USB...\r\n");

    if (usb_reset() != CH376S_CMD_RET_SUCCESS)
    {
        serial_puts("Failed to reset USB module\r\n");
        return;
    }

    size_t kernel_size = file_read("KERNEL.BIN", (uint8_t *)0x80000);

    if (kernel_size == 0)
    {
        serial_puts("Failed to load kernel file.\r\n");
        return;
    }

    serial_puts("Kernel loaded at 0x80000\r\n");

    asm("jsr 0x80000");
}

void command_not_found(char *command_name)
{
    serial_puts("Command not found: ");
    serial_puts(command_name);
}

uint8_t readline(char *buffer)
{
    uint8_t count = 0;
    uint8_t in = serial_getc();

    while (in != '\n' && in != '\r')
    {
        // Character is printable ASCII
        if (in >= 0x20 && in < 0x7F)
        {
            serial_putc(in);

            buffer[count] = in;
            count++;
        }

        in = serial_getc();
    }

    buffer[count] = 0;

    return count;
}
