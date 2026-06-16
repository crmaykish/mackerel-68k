#include "uart.h"
#include "uart_xr68c681.h"

void uart_init(void)
{
    MEM(DUART1_IMR) = 0x00;        // Mask all interrupts
    MEM(DUART1_OPCR) = 0x00;       // OP0/OP1 = RTSA//RTSB/, OP2-OP7 = general-purpose GPIO
    MEM(DUART1_MR1B) = 0b00010011; // No Rx RTS, No Parity, 8 bits per character
    MEM(DUART1_MR2B) = 0b00000111; // Channel mode normal, No Tx RTS, No CTS, stop bit length 1
    MEM(DUART1_ACR) = 0x80;        // Baudrate set 2
    MEM(DUART1_CRB) = 0x80;        // Set Rx extended bit
    MEM(DUART1_CRB) = 0xA0;        // Set Tx extended bit
    MEM(DUART1_CSRB) = 0x88;       // 115200 baud
    MEM(DUART1_CRB) = 0b0101;      // Enable Tx/Rx
}

void uart_putc(char c)
{
    while ((MEM(DUART1_SRB) & 0b00000100) == 0) {} // wait until TX is ready
    MEM(DUART1_TBB) = c;
}

char uart_getc(void)
{
    while ((MEM(DUART1_SRB) & 0b00000001) == 0) {} // wait until a byte arrives
    return MEM(DUART1_RBB);
}

bool uart_rx_ready(void)
{
    return (MEM(DUART1_SRB) & 0b00000001) != 0;
}
