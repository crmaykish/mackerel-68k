#include <stdio.h>
#include <stdbool.h>
#include "mackerel.h"

void __attribute__((interrupt)) duart_isr()
{
    // Determine the type of interrupt
    uint8_t misr = MEM(DUART_MISR);

    if (misr & DUART_INTR_RXRDY)
    {
        // RX character available
        uint8_t a = MEM(DUART_RBB); // Read the available byte. This clears the interrupt
        duart_putc(a);              // Print the character back out
    }
}

void __attribute__((interrupt)) duart1_isr()
{
    // Determine the type of interrupt
    uint8_t misr = MEM(DUART1_MISR);

    if (misr & DUART_INTR_COUNTER)
    {
        MEM(DUART1_OPR_RESET);
        duart_putc('y');
    }
}

int main()
{
    uint32_t i = 0;

    printf("Starting Mackerel kernel...\n");

    // Map an exception handler for the periodic timer interrupt
    set_exception_handler(65, &duart_isr);
    set_exception_handler(66, &duart1_isr);

    // Setup DUART0
    MEM(DUART_IVR) = 65;                                    // Set interrupt base register
    MEM(DUART_IMR) = DUART_INTR_RXRDY; // Unmask interrupts

    printf("Starting kernel...%X\r\n", 0xC0FFEE);

    // Turn interrupts on
    set_interrupts(true);

    while (true)
    {
        printf("Loop: %X\r\n", i);

        i++;

        delay(50000);
    }

    return 0;
}
