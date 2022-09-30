#include <stdio.h>
#include <stdbool.h>
#include "mackerel.h"

#define DUART_BASE_REGISTER 0x40

void __attribute__((interrupt)) duart_isr()
{
    // Determine the type of interrupt
    uint8_t misr = MEM(DUART_MISR);

    if (misr & DUART_INTR_COUNTER)
    {
        // Counter timed out
        MEM(DUART_OPR_RESET); // Read the OPR port to clear the counter and reset the timer. This clears the interrupt

        mputc('X'); // Print an X when the timer ticks
    }

    if (misr & DUART_INTR_RXRDY)
    {
        // RX character available
        uint8_t a = MEM(DUART_RBB); // Read the available byte. This clears the interrupt
        mputc(a);                   // Print the character back out
    }
}

int main()
{
    uint32_t i = 0;

    // Map an exception handler for the periodic timer interrupt
    set_exception_handler(DUART_BASE_REGISTER, &duart_isr);

    // NOTE: Interrupts are not working with classic DUART

    // Setup DUART timer as 50 Hz interrupt
    MEM(DUART_IVR) = DUART_BASE_REGISTER;                     // Set interrupt base register
    MEM(DUART_ACR) = 0xF0;                                    // Set timer mode X/16
    MEM(DUART_CUR) = 0x09;                                    // Counter upper byte, (3.6864MHz / 2 / 16 / 0x900) = 50 Hz
    MEM(DUART_CLR) = 0x00;                                    // Counter lower byte
    MEM(DUART_IMR) = (DUART_INTR_COUNTER | DUART_INTR_RXRDY); // Unmask interrupts
    MEM(DUART_OPR);                                           // Start counter

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
