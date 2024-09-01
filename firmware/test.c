#include "mackerel.h"


int main()
{
    unsigned char a = 0;

    set_gpio(0x55);

    // duart_init();

    // not even getting through init?

    while (1)
    {
        // duart_putc('A');

        set_leds(a);
        // set_gpio(MEM(DUART1_SRB));
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
