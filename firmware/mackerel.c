#include "mackerel.h"

void serial_init()
{
    MEM(ACIA_STATUS) = 0;
    MEM(ACIA_COMMAND) = 0x0B;
    MEM(ACIA_CONTROL) = 0b00011110; // 9600 baud
}

void serial_putc(char a)
{
    while ((MEM(ACIA_STATUS) & ACIA_TX_READY) == 0)
    {
    }

    MEM(ACIA_DATA) = a;

    if (a == '\n')
    {
        serial_putc('\r');
    }
}

void serial_puts(const char *s)
{
    unsigned i = 0;

    while (s[i] != 0)
    {
        serial_putc(s[i]);
        i++;
    }
}

char serial_getc()
{
    while ((MEM(ACIA_STATUS) & ACIA_RX_READY) == 0)
    {
    }

    return MEM(ACIA_DATA);
}

void mfp_init()
{
    MEM(MFP_DDR) = 0xFF;        // Set GPIO direction to output
    MEM(MFP_TACR) = 0b00010001; // timer A enabled, delay mode, /4 prescalar
    MEM(MFP_TADR) = 12;          // 19.2 kHz square wave on timer A
}

void mfp_putc(char s)
{
    while ((MEM(MFP_TSR) & 0b10000000) == 0)
    {
    }

    MEM(MFP_UDR) = s;

    if (s == '\n')
    {
        mfp_putc('\r');
    }
}

void mfp_puts(const char *s)
{
    unsigned i = 0;

    while (s[i] != 0)
    {
        mfp_putc(s[i]);
        i++;
    }
}

char mfp_getc()
{
    while ((MEM(MFP_RSR) & 0b10000000) == 0)
    {
    }

    return MEM(MFP_UDR);
}