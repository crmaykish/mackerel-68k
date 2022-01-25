#include "mackerel.h"
#include <stdarg.h>
#include <stdio.h>

void m_init()
{
    MEM(ACIA_STATUS) = 0;
    MEM(ACIA_COMMAND) = 0x0B;
    MEM(ACIA_CONTROL) = 0b00011110; // 9600 baud
}

void m_putc(char a)
{
    while ((MEM(ACIA_STATUS) & ACIA_TX_READY) == 0)
    {
    }

    MEM(ACIA_DATA) = a;
}

void m_puts(const char *s)
{
    unsigned i = 0;

    while (s[i] != 0)
    {
        m_putc(s[i]);
        i++;
    }
}

char m_getc()
{
    while ((MEM(ACIA_STATUS) & ACIA_RX_READY) == 0)
    {
    }

    return MEM(ACIA_DATA);
}

void m_readline(char *buffer)
{
    int count = 0;
    char in = m_getc();

    while (in != '\n' && in != '\r')
    {
        // Character is printable ASCII
        if (in >= 0x20 && in < 0x7F)
        {
            m_putc(in);

            buffer[count] = in;
            count++;
        }

        in = m_getc();
    }

    buffer[count] = 0;
}

int m_printf(const char *fmt, ...)
{
    char buffer[64];
    int ret;

    va_list myargs;
    va_start(myargs, fmt);

    ret = vsiprintf(buffer, fmt, myargs);

    m_puts(buffer);

    va_end(myargs);

    return ret;
}
