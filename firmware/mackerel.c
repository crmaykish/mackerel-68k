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

void serial_readline(char *buffer)
{
    int count = 0;
    char in = serial_getc();

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
}

void mfp_init()
{
    MEM(MFP_DDR) = 0xFF;       // Set GPIO direction to output
    MEM(MFP_UCR) = 0b10001000; // 1/16 clock, 8 data, 1 stop bit, no parity
    MEM(MFP_TSR) = 1;          // Enable transmitter
    MEM(MFP_RSR) = 1;          // Enable receiver

    // char w = MEM(MFP_UCR);
}

void mfp_putc(char s)
{
    while ((MEM(MFP_TSR) & 0b10000000) == 0)
    {
    }

    MEM(MFP_UDR) = s;
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

void mfp_readline(char *buffer)
{
    int count = 0;
    char in = mfp_getc();

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
}
