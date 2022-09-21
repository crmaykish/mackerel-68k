#include <stdio.h>
#include <stdbool.h>
#include "mackerel.h"

// uint32_t a = 0;

void __attribute__((interrupt)) system_timer_intr()
{
    uint8_t a = MEM(DUART_OPR_RESET); // stop counter, i.e. reset the timer

    mputc('X');
}

int main()
{
    uint32_t i = 0;

    // Map an exception handler for the periodic timer interrupt
    set_exception_handler(0x40, &system_timer_intr);

    MEM(DUART_IVR) = 0x40; // Interrupt base register
    MEM(DUART_ACR) = 0xF0;       // set timer mode X/16
    MEM(DUART_IMR) = 0b00001000; // unmask counter interrupt
    MEM(DUART_CUR) = 0x09;       // Counter upper byte, (3.6864MHz / 2 / 16 / 0x900) = 100Hz
    MEM(DUART_CLR) = 0x00;       // Counter lower byte

    uint8_t a = MEM(DUART_OPR); // start counter


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
