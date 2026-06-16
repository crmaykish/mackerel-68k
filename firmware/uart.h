#ifndef _UART_H
#define _UART_H

#include <stdbool.h>

void uart_init(void);
void uart_putc(char c);
char uart_getc(void);
bool uart_rx_ready(void);

#endif
