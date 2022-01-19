#include "mackerel.h"

int main()
{
    char buffer[64];

    mack_acia_init();

    acia_puts("Mackerel 68k\r\n");

    while (1)
    {
        acia_puts("> ");
        acia_readline(buffer);

        acia_puts("\r\n");

        acia_puts("You entered: ");

        acia_puts(buffer);

        acia_puts("\r\n");
    }
}
