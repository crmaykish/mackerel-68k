#include "mackerel.h"


int main()
{
    unsigned char a = 0;

    set_gpio(0x55);

    duart_init();

    set_gpio(0b11100000);

    while (1)
    {
        duart_putc('B');

        set_leds(a);
        a++;
    }

    return 0;
}

void _start()
{
    // asm("or.w #0x700, %sr");

    // Call main
    main();
}
