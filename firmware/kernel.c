#include <stdio.h>
#include <stdbool.h>
#include "mackerel.h"

uint32_t a = 0;

void __attribute__((interrupt)) cpld_timer_intr()
{
    // Invert the OP0 pin on the DUART
    MEM(DUART_OPR_RESET) = a;
    a++;
    MEM(DUART_OPR) = a;
}

int main()
{
    uint32_t i = 0;

    // Map an exception handler for the periodic timer interrupt
    set_exception_handler(25, &cpld_timer_intr);

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
