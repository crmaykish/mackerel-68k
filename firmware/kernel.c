#include <stdio.h>
#include <stdbool.h>
#include "mackerel.h"

#define DUART_BASE_REGISTER 0x40
#define DUART1_BASE_REGISTER 0x50

void __attribute__((interrupt)) duart_isr()
{
    // Determine the type of interrupt
    uint8_t misr = MEM(DUART_MISR);

    if (misr & DUART_INTR_RXRDY)
    {
        // RX character available
        uint8_t a = MEM(DUART_RBB); // Read the available byte. This clears the interrupt
        mputc(a);                   // Print the character back out
    }
}

void __attribute__((interrupt)) duart1_isr()
{
    // Determine the type of interrupt
    uint8_t misr = MEM(DUART1_MISR);

    if (misr & DUART_INTR_COUNTER)
    {
        // Counter timed out
        MEM(DUART1_OPR_RESET); // Read the OPR port to clear the counter and reset the timer. This clears the interrupt

        mputc('Y'); // Print a Y when the timer ticks
    }
}

int main()
{
    uint32_t i = 0;

    // Map an exception handler for the periodic timer interrupt
    set_exception_handler(DUART_BASE_REGISTER, &duart_isr);
    set_exception_handler(DUART1_BASE_REGISTER, &duart1_isr);

    // Setup DUART0
    MEM(DUART_IVR) = DUART_BASE_REGISTER; // Set interrupt base register
    MEM(DUART_IMR) = DUART_INTR_RXRDY;    // Unmask interrupts
    MEM(DUART_OPR);                       // Start counter

    // Setup DUART1
    MEM(DUART1_IVR) = DUART1_BASE_REGISTER; // Set interrupt base register
    MEM(DUART1_ACR) = 0xF0;                 // Set timer mode X/16
    MEM(DUART1_CUR) = 0x80;                 // Counter upper byte, (3.6864MHz / 2 / 16 / 0x900) = 50 Hz
    MEM(DUART1_CLR) = 0x00;                 // Counter lower byte
    MEM(DUART1_IMR) = DUART_INTR_COUNTER;   // Unmask interrupts
    MEM(DUART1_OPR);

    printf("Starting kernel...%X\r\n", 0xC0FFEE);

    // Turn interrupts on
    set_interrupts(true);

    while (true)
    {
        printf("Loop: %X\r\n", i);

        i++;

        delay(100000);
    }

    return 0;
}
