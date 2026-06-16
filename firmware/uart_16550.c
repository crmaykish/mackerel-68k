#include "uart.h"
#include "uart_16550.h"

void uart_init(void)
{
    MEM(UART_LCR) = 0x80; // DLAB=1 -> divisor latches visible
    MEM(UART_DLL) = 35;   // 64.8MHz / (16*35) = 115714  (~115200, +0.45%)
    MEM(UART_DLM) = 0;
    MEM(UART_LCR) = 0x03; // DLAB=0, 8 data bits / no parity / 1 stop (8N1)
    MEM(UART_FCR) = 0x07; // enable + clear RX & TX FIFOs
}

void uart_putc(char c)
{
    while (!(MEM(UART_LSR) & LSR_THRE)) {}
    MEM(UART_THR) = c;
}

char uart_getc(void)
{
    while (!(MEM(UART_LSR) & LSR_DR)) {}
    return MEM(UART_RBR);
}

bool uart_rx_ready(void)
{
    return (MEM(UART_LSR) & LSR_DR) != 0;
}
