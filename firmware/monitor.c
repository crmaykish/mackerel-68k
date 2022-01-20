#include "mackerel.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

bool is_prime(int p)
{
    int i;

    if (p < 2)
    {
        return false;
    }
    else if (p == 2)
    {
        return true;
    }
    else
    {
        for (i = 2; i < (p / 2) + 1; i++)
        {
            if (p % i == 0)
            {
                return false;
            }
        }
    }

    return true;
}

void primes()
{
    int count = 0;

    m_printf("Calculating primes:\r\n");

    for (int i = 1; i < 10000; i++)
    {
        if (is_prime(i))
        {
            count++;

            m_printf("%d: %d\r\n", count, i);
        }
    }
}

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
        else if (strncmp(input, "prime", 5) == 0)
        {
            primes();
        }
        else
        {
            m_printf("You entered: %s", input);
        }
    }

    return 0;
}
