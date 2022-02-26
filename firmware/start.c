#include "mackerel.h"

extern int main();

void __attribute((interrupt)) unhandled_exception()
{
    // Display a pattern on the MFP LEDs and loop forever
    MEM(MFP_GPDR) = 0xAA;

    while (1)
    {
    }
}

void _start()
{
    // TODO: Setup .bss and .data sections correctly in RAM

    // Disable interrupts
    set_interrupts(false);

    // Initialize the vector table at the start of RAM
    for (int i = 0; i < VECTOR_TABLE_SIZE; i += 1)
    {
        // All exceptions start off pointing to a known handler
        set_exception_handler(i, &unhandled_exception);
    }

    // Enable interrupts
    set_interrupts(true);

    // Setup the hardware peripherals
    mfp_init();

    // Call main
    main();
}
