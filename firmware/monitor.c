#include "mackerel.h"
#include <string.h>
#include <stdio.h>

int main()
{
    char input[64];

    m_printf("Mackerel 68k");

    while (1)
    {
        m_printf("\r\n> ");
        m_readline(input);

        m_printf("\r\n");

        if (strncmp(input, "help", 4) == 0)
        {
            m_printf("Help");
        }
        else
        {
            m_printf("You entered: %s", input);
        }
    }

    return 0;
}
