#include "mackerel.h"

void mack_acia_init()
{
    MEM(ACIA_STATUS) = 0;
    MEM(ACIA_COMMAND) = 0x0B;
    MEM(ACIA_CONTROL) = 0b00010000;
}

int acia_putc(int a)
{
    while ((MEM(ACIA_STATUS) & ACIA_TX_READY) == 0)
    {
    }

    MEM(ACIA_DATA) = a;

    return a;
}

int acia_puts(const char *s)
{
    unsigned i = 0;

    while (s[i] != 0)
    {
        acia_putc(s[i]);
        i++;
    }

    return 0;
}

int acia_getc()
{
    while ((MEM(ACIA_STATUS) & ACIA_RX_READY) == 0)
    {
    }

    return MEM(ACIA_DATA);
}

void acia_readline(char *buffer)
{
    int count = 0;
    char in = acia_getc();

    while (in != '\n' && in != '\r')
    {
        // Character is printable ASCII
        if (in >= 0x20 && in < 0x7F)
        {
            acia_putc(in);

            buffer[count] = in;
            count++;
        }

        in = acia_getc();
    }

    buffer[count] = 0;
}