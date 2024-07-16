#include "mackerel.h"

#define ENABLE_XR68C681

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

void duart_init(void)
{
    MEM(DUART1_IMR) = 0x00;        // Mask all interrupts
    MEM(DUART1_MR1B) = 0b00010011; // No Rx RTS, No Parity, 8 bits per character
    MEM(DUART1_MR2B) = 0b00000111; // Channel mode normal, No Tx RTS, No CTS, stop bit length 1
    MEM(DUART1_ACR) = 0x80;        // Baudrate set 2
    MEM(DUART1_CRB) = 0x80;        // Set Rx extended bit
    MEM(DUART1_CRB) = 0xA0;        // Set Tx extended bit
    MEM(DUART1_CSRB) = 0x88;       // 115200 baud
    MEM(DUART1_CRB) = 0b0101;      // Enable Tx/Rx
}

void duart_putc(char c)
{
    while ((MEM(DUART1_SRB) & 0b00000100) == 0)
    {
    }

    MEM(DUART1_TBB) = c;

    if (c == 0x0A)
    {
        duart_putc(0x0D);
    }
}

char duart_getc(void)
{
    while ((MEM(DUART1_SRB) & 0b00000001) == 0)
    {
    }

    return MEM(DUART1_RBB);
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
