#include "mackerel.h"

int main()
{
    unsigned char a = 0;

    MEM(0xF00000) = 0x01;

    MEM(DUART1_IMR) = 0x00;        // Mask all interrupts
    MEM(0xF00000) = 0x2;
    MEM(DUART1_MR1B) = 0b00010011; // No Rx RTS, No Parity, 8 bits per character
    MEM(0xF00000) = 0x3;
    MEM(DUART1_MR2B) = 0b00000111; // Channel mode normal, No Tx RTS, No CTS, stop bit length 1
    MEM(0xF00000) = 0x4;
    MEM(DUART1_ACR) = 0x80;        // Baudrate set 2
    MEM(0xF00000) = 0x5;
    MEM(DUART1_CRB) = 0x80;        // Set Rx extended bit
    MEM(0xF00000) = 0x6;
    MEM(DUART1_CRB) = 0xA0;        // Set Tx extended bit
    MEM(0xF00000) = 0x7;
    MEM(DUART1_CSRB) = 0x88;       // 115200 baud
    MEM(DUART1_CRB) = 0b0101;      // Enable Tx/Rx

    MEM(0xF00000) = 0x2;

    while (1)
    {
        // duart_putc('A');

        while ((MEM(DUART1_SRB) & 0b00000100) == 0)
        {
        }

        MEM(DUART1_TBB) = 'A';

        MEM(0xF00000) = a;
        a++;
    }

    return 0;
}

void _start()
{
    // Call main
    main();
}
