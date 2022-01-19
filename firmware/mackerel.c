#include "mackerel.h"

void mack_acia_init()
{
    MEM(ACIA_STATUS) = 0;
    MEM(ACIA_COMMAND) = 0x0B;
    MEM(ACIA_CONTROL) = 0b00010000;
}

void putc(unsigned char a)
{
    while ((MEM(ACIA_STATUS) & ACIA_TX_READY) == 0)
    {
    }

    MEM(ACIA_DATA) = a;
}

void puts(unsigned char *s)
{
    unsigned i = 0;

    while (s[i] != 0)
    {
        putc(s[i]);
        i++;
    }
}

unsigned char getc()
{
    while ((MEM(ACIA_STATUS) & ACIA_RX_READY) == 0)
    {
    }

    return MEM(ACIA_DATA);
}

void readline(char *buffer)
{
    int count = 0;
    char in = getc();

    while (in != '\n' && in != '\r')
    {
        // Character is printable ASCII
        if (in >= 0x20 && in < 0x7F)
        {
            putc(in);

            buffer[count] = in;
            count++;
        }

        in = getc();
    }

    buffer[count] = 0;
}