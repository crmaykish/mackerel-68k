#include <stdio.h>
#include <stdbool.h>
#include "mackerel.h"

void __attribute__((interrupt)) duart1_isr()
{
    // Determine the type of interrupt
    uint8_t misr = MEM(DUART1_MISR);

    if (misr & DUART_INTR_RXRDY)
    {
        // RX character available
        uint8_t a = MEM(DUART1_RBB); // Read the available byte. This clears the interrupt
        duart_putc(a);               // Print the character back out
    }
    else if (misr & DUART_INTR_COUNTER)
    {
        MEM(DUART1_OPR_RESET);
        duart_putc('y');
    }
}

void __attribute__((interrupt)) timer_isr()
{
    duart_putc('x'); 
}

int main()
{
    uint32_t i = 0;

    printf("Starting Mackerel kernel...\n");

    // Map an exception handler for the periodic timer interrupt
    set_exception_handler(65, &duart1_isr);
    // set_exception_handler(66, &duart2_isr);

    // Setup DUART 1
    MEM(DUART1_IVR) = 65; // Set interrupt base register

    // Setup DUART 1 timer as 5 Hz interrupt
    MEM(DUART1_ACR) = 0xF0;       // Set timer mode X/16
    MEM(DUART1_IMR) = 0b00001000; // Unmask counter interrupt
    MEM(DUART1_CUR) = 0x5A;       // Counter upper byte, (3.6864MHz / 2 / 16 / 0x5A00) = 5 Hz
    MEM(DUART1_CLR) = 0x00;       // Counter lower byte
    MEM(DUART1_OPR);              // Start counter

    MEM(DUART1_IMR) = DUART_INTR_RXRDY; // Unmask interrupts

    set_exception_handler(28, &timer_isr);

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
