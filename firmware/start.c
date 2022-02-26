#include "mackerel.h"

extern int main();

void _start()
{
    // TODO: Setup .bss and .data sections correctly in RAM

    // Copy the vector table into the beginning of RAM
    for (int i = 0; i < VECTOR_TABLE_SIZE; i++)
    {
        MEM(RAM_START + i) = MEM(ROM_START + i);
    }

    // Setup the hardware peripherals
    mfp_init();

    // Call main
    main();
}
