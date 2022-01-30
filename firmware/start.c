#include "mackerel.h"

extern int main();

void _start()
{
    // TODO: Setup .bss and .data sections correctly in RAM

    // Setup the hardware peripherals
    serial_init();
    mfp_init();

    // Call main
    main();
}
