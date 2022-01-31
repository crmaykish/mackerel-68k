
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "mackerel.h"

#define INPUT_BUFFER_SIZE 32

uint8_t readline(char *buffer);
void handler_run();
void handler_load();
void handler_print();
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
        else if (strncmp(buffer, "print", 5) == 0)
        {
            handler_print();
        }
        else
        {
            command_not_found(buffer);
        }

        mfp_puts("\r\n");
    }

    return 0;
}

void handler_print()
{
    mfp_puts((char *)0x80000);
}

void handler_run()
{
    mfp_puts("Run loaded program\r\n");
    asm("jsr 0x80000");
}

void handler_load()
{
    int in_count = 0;
    int magic_count = 0;
    uint8_t in = 0;

    mfp_puts("Loading from serial...\r\n");

    while (magic_count != 3)
    {
        in = mfp_getc();

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
