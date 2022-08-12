#include "mackerel.h"

int main()
{
    duart_puts("Echo test:\r\n");

    while (1)
    {
        char a = duart_getc();
        duart_putc(a);
    }

    return 0;
}
