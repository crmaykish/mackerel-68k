#ifndef _UART_16550_H
#define _UART_16550_H

#include "mackerel.h"

#define UART_THR (UART_BASE + 0)  // write: TX holding   (DLAB=0)
#define UART_RBR (UART_BASE + 0)  // read:  RX buffer    (DLAB=0)
#define UART_DLL (UART_BASE + 0)  // DLAB=1: divisor low
#define UART_IER (UART_BASE + 2)  // interrupt enable    (DLAB=0)
#define UART_DLM (UART_BASE + 2)  // DLAB=1: divisor high
#define UART_IIR (UART_BASE + 4)  // read:  interrupt identification
#define UART_FCR (UART_BASE + 4)  // write: FIFO control
#define UART_LCR (UART_BASE + 6)  // line control (DLAB is bit 7)
#define UART_LSR (UART_BASE + 10) // line status

// Line Status Register bits
#define LSR_DR 0x01    // data ready: an RX byte is waiting in RBR/FIFO
#define LSR_THRE 0x20  // TX holding register empty: ok to write THR

// Interrupt Enable Register bits
#define IER_RDA 0x01   // received-data-available (+ char-timeout) interrupt
#define IER_THRE 0x02  // transmitter-holding-register-empty interrupt

// Interrupt Identification Register bits
#define IIR_NOT_PENDING 0x01 // bit0: 1 = no interrupt pending (0 = pending)

#endif
