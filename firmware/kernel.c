#include <stdio.h>
#include <stdbool.h>
#include "mackerel.h"

// uint32_t a = 0;

void __attribute__((interrupt)) system_timer_intr()
{
    // // Invert the OP0 pin on the DUART
    // MEM(DUART_OPR_RESET) = a;
    // a++;
    // MEM(DUART_OPR) = a;

    mputc('X');

    while (1)
    {
    }
}

int main()
{
    uint32_t i = 0;

    // Map an exception handler for the periodic timer interrupt
    set_exception_handler(0x40, &system_timer_intr);

    // // Set MFP Timer B to run at 36 Hz and trigger an interrupt on every tick
    // MEM(MFP_TBDR) = 0;         // Timer B counter max (i.e 255);
    // MEM(MFP_TBCR) = 0b0010111; // Timer B enabled, delay mode, /200 prescalar
    // MEM(MFP_VR) = 0x40;        // Set base vector for MFP interrupt handlers
    // MEM(MFP_IERA) = 0x01;      // Enable interrupts for Timer B
    // MEM(MFP_IMRA) = 0x01;      // Unmask interrupt for Timer B

    MEM(DUART_IVR) = 0x40; // Interrupt base register
    // MEM(DUART_ACR) = 0xF0;       // set timer mode X/16
    MEM(DUART_IMR) = 0b00001000; // unmask counter interrupt
    MEM(DUART_CUR) = 0x00;       // Counter upper byte
    MEM(DUART_CLR) = 0x80;       // Counter lower byte

    uint8_t a = MEM(DUART_OPR); // start counter

    MEM(DUART_OPR) = 0;

    printf("Starting kernel...%X\r\n", 0xC0FFEE);

    // Turn interrupts on
    set_interrupts(true);

    while (true)
    {
        printf("Loop: %X\r\n", i);

        i++;

        delay(1000);
    }

    return 0;
}
