#include "mackerel.h"

extern int main();

void _start()
{
    // TODO: Setup .bss and .data sections correctly in RAM

    // Setup the serial port
    mack_acia_init();

    // Call main
    main();

    while (1)
    {
    }
}
