// #include "mackerel.h"

int main()
{
    // duart_init();

    unsigned char a = 0;

    // duart_init();

    while (1)
    {
        // duart_putc('A');

        *(volatile unsigned char *)(0xF00000) = a;
        a++;
    }

    return 0;
}

void _start()
{
    // Call main
    main();
}
