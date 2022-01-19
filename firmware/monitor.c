#include "mackerel.h"
#include <string.h>

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

        if (strncmp(buffer, "help", 4) == 0)
        {
            acia_puts("Help");
        }
        else
        {
            acia_puts("You entered: ");
            acia_puts(buffer);
        }

        acia_puts("\r\n");
    }
}
