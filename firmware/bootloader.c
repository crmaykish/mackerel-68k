#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "mackerel.h"

#define INPUT_BUFFER_SIZE 32

uint8_t readline(char *buffer);
void handler_run();
void handler_load();
void command_not_found(char *command);

char buffer[INPUT_BUFFER_SIZE];

int main()
{
    serial_puts("\r\n### Mackerel-8 Bootloader ###\r\n");

    while (true)
    {
        // Present the command prompt and wait for input
        serial_puts("> ");
        readline(buffer);
        serial_puts("\r\n");

        if (strncmp(buffer, "load", 4) == 0)
        {
            handler_load();
        }
        else if (strncmp(buffer, "run", 3) == 0)
        {
            handler_run();
        }
        else
        {
            command_not_found(buffer);
        }

        serial_puts("\r\n");
    }

    return 0;
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

    serial_puts("Done!");
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
