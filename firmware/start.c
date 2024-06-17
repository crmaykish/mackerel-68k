#include "mackerel.h"

extern int main();

void _start()
{
    // Disable interrupts
    set_interrupts(false);

    // Setup the hardware peripherals
    // mfp_init();
    duart_init();

    // Call main
    main();
}
