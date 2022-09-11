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

void duart_init()
{
    // Reset port A (probably not necessary)
    MEM(DUART_CRA) = 0b0010000; // Reset MR Pointer to MR1
    MEM(DUART_CRA) = 0b0100000; // Reset receiver
    MEM(DUART_CRA) = 0b0110000; // Reset transmitter
    MEM(DUART_CRA) = 0b1000000; // Reset error status

    MEM(DUART_IMR) = 0x00;            // Mask all interrupts
    MEM(DUART_MR1A) = 0b00010011;     // No Rx RTS, No Parity, 8 bits per character
    MEM(DUART_MR2A) = 0b00000111;     // Channel mode normal, No Tx RTS, No CTS, stop bit length 1
    MEM(DUART_ACR) = 0x60;            // Set mode to Timer, external clock
    unsigned char x = MEM(DUART_CRA); // Enabled undocumented baudrates
    MEM(DUART_CSRA) = 0x66;           // 1200 baud, something about clock???
    MEM(DUART_CUR) = 0x00;            // Counter high 0
    MEM(DUART_CLR) = 0x02;            // Counter low 2
    unsigned char y = MEM(DUART_OPR); // Start counter
    MEM(DUART_CRA) = 0b0101;          // Enable Tx/Rx
}

void duart_putc(char c)
{
    while ((MEM(DUART_SRA) & 0b00000100) == 0)
    {
    }

    if (c == '\n')
    {
        duart_putc('\r');
    }

    MEM(DUART_TBA) = c;
}

char duart_getc()
{
    while ((MEM(DUART_SRA) & 0b00000001) == 0)
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
