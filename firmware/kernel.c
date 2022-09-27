#include <stdio.h>
#include <stdbool.h>
#include "mackerel.h"

void __attribute__((interrupt)) system_timer_intr()
{
    MEM(DUART_OPR_RESET); // Stop counter, i.e. reset the timer

    // TODO: Interrupt handler code goes here
}

int main()
{
    uint32_t i = 0;

    // Map an exception handler for the periodic timer interrupt
    set_exception_handler(0x40, &system_timer_intr);

    // NOTE: Interrupts don't work with classic DUART

    // Setup DUART timer as 50 Hz interrupt
    MEM(DUART_IVR) = 0x40;       // Interrupt base register
    MEM(DUART_ACR) = 0xF0;       // Set timer mode X/16
    MEM(DUART_IMR) = 0b00001000; // Unmask counter interrupt
    MEM(DUART_CUR) = 0x09;       // Counter upper byte, (3.6864MHz / 2 / 16 / 0x900) = 50 Hz
    MEM(DUART_CLR) = 0x00;       // Counter lower byte
    MEM(DUART_OPR);              // Start counter

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
