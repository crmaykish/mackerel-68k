#include "mackerel.h"

void set_interrupts(bool enabled)
{
    if (enabled)
    {
        // Set minimum interrupt level to 0 in the status register
        asm("and.w #0xF8FF, %sr");
    }
    else
    {
        // Set minimum interrupt level to 7, i.e. only non-maskable interrupts enabled
        asm("or.w #0x700, %sr");
    }
}

void set_exception_handler(unsigned char exception_number, void (*exception_handler)())
{
    *((int *)(exception_number * 4)) = (int)exception_handler;
}

void mfp_init()
{
    MEM(MFP_DDR) = 0xFF; // Set GPIO direction to output

    // Set up timer A as the UART clock (pin TA0 is connected to TC and RC)
    MEM(MFP_TACR) = 0b00010001; // timer A enabled, delay mode, /4 prescalar
    MEM(MFP_TADR) = 3;          // 153.6 kHz square wave on timer A (with 3.6864 MHz oscillator)

    // Set up UART
    MEM(MFP_UCR) = 0b10001000; // UART uses 1/16 clock rate, 8 data, 1 stop bit, no parity
    MEM(MFP_TSR) = 1;          // Enable transmitter
    MEM(MFP_RSR) = 1;          // Enable receiver
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

void duart_init()
{
    MEM(DUART_IMR) = 0x00; // Mask all interrupts
    MEM(DUART_MR1A) = 0x93;
    MEM(DUART_MR2A) = 0x07;
    MEM(DUART_ACR) = 0x60;
    unsigned char x = MEM(DUART_CRA); // Enabled undocumented baudrates
    MEM(DUART_CSRA) = 0x66;
    MEM(DUART_CUR) = 0x00;
    MEM(DUART_CLR) = 0x02;
    unsigned char y = MEM(DUART_OPR); // Start counter
    MEM(DUART_CRA) = 0b101;           // Enable Tx/Rx
    MEM(DUART_OPR_RESET) = 0xFF;
    MEM(DUART_OPR) = 0x00;
}

void duart_putc(char c)
{
    while ((MEM(DUART_SRA) & 0b00000001) == 0)
    {
    }

    MEM(DUART_TBA) = c;
}

char duart_getc()
{
    while ((MEM(DUART_SRA) & 0b00001000) == 0)
    {
    }

    return MEM(DUART_RBA);
}

void duart_puts(const char *s)
{
    unsigned i = 0;

    while (s[i] != 0)
    {
        duart_putc(s[i]);
        i++;
    }
}

void delay(int time)
{
    for (int delay = 0; delay < time; delay++)
        __asm__ __volatile__("");
}
