#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "mackerel.h"

// TODO: get printf working for real
#define printf(s...) m_printf(s)

#define INPUT_BUFFER_SIZE 32
#define COMMAND_MAX_LENGTH 6
#define MEMDUMP_BYTES 256

typedef struct
{
    char name[6];
    char param1[6];
    char param2[6];
    char desc[48];
    void (*handler)();
} command_t;

uint8_t readline(char *buffer);
void memdump(uint8_t *address, uint16_t bytes);

// Handler functions for each monitor command
void handler_dump();
void handler_run();
void handler_load();
void command_not_found(char *command_name);

// Input command definitions
static const command_t commands[] = {
    {"dump", "addr", "", "Dump memory in hex and ASCII", handler_dump},
    {"run", "", "", "Jump to program RAM (0x81000)", handler_run},
    {"load", "addr", "", "Load a program over serial", handler_load}};

static const uint8_t COMMAND_COUNT = sizeof(commands) / sizeof(command_t);

void print_string_bin(char *str, uint8_t max)
{
    uint8_t i = 0;

    while (i < max)
    {
        if (str[i] >= 32 && str[i] < 127)
        {
            m_putc(str[i]);
        }
        else
        {
            m_putc('.');
        }

        i++;
    }
}

char buffer[INPUT_BUFFER_SIZE];

int main()
{
    uint8_t i;

    char *command;
    bool command_handled;

    printf("\r\n### Mackerel-8 Bootloader ###\r\n");

    while (true)
    {
        command_handled = false;

        // Present the command prompt and wait for input
        printf("> ");
        readline(buffer);
        printf("\r\n");

        command = strtok(buffer, " ");

        // Look for the command name in the command list
        for (i = 0; i < COMMAND_COUNT; i++)
        {
            if (strncmp(command, commands[i].name, COMMAND_MAX_LENGTH) == 0)
            {
                // Found the command, handle it
                commands[i].handler();
                command_handled = true;
                break;
            }
        }

        if (!command_handled)
        {
            command_not_found(command);
        }

        printf("\r\n");
    }

    return 0;
}

void handler_dump()
{
    memdump((uint8_t *)0x81000, MEMDUMP_BYTES);
}

void handler_run()
{
    printf("Run loaded program\r\n");
    asm("jsr 0x81000");
}

void handler_load()
{
    int in_count = 0;
    int magic_count = 0;
    uint8_t in = 0;

    printf("Loading into: 0x%06X...", 0x81000);

    while (magic_count != 3)
    {
        in = m_getc();

        MEM(0x81000 + in_count) = in;

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

    printf("%d bytes read\r\nDone!", in_count - 3);
}

void command_not_found(char *command_name)
{
    printf("Command not found: ");
    printf(command_name);
}

uint8_t readline(char *buffer)
{
    uint8_t count = 0;
    uint8_t in = m_getc();

    while (in != '\n' && in != '\r')
    {
        // Character is printable ASCII
        if (in >= 0x20 && in < 0x7F)
        {
            m_putc(in);

            buffer[count] = in;
            count++;
        }

        in = m_getc();
    }

    buffer[count] = 0;

    return count;
}

void memdump(uint8_t *address, uint16_t bytes)
{
    uint32_t i = 0;
    uint8_t b = 0;

    printf("%06X  ", address);

    while (i < bytes)
    {
        b = *(address + i);
        printf("%02X ", b);

        i++;

        if (i % 16 == 0 && i < bytes)
        {
            printf(" |");
            print_string_bin((address + i - 16), 16);

            printf("|\r\n%06X  ", address + i);
        }
        else if (i % 8 == 0)
        {
            m_putc(' ');
        }
    }

    m_putc('|');
    print_string_bin((address + i - 16), 16);
    m_putc('|');
}
