#include "console.h"
#include "uart.h"

void console_init(void)
{
    uart_init();
}

void console_putc(char c)
{
    if (c == '\n')
        uart_putc('\r');

    uart_putc(c);
}

char console_getc(void)
{
    return uart_getc();
}

void console_puts(const char *s)
{
    while (*s)
        console_putc(*s++);
}
