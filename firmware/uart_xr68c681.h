#ifndef _UART_XR68C681_H
#define _UART_XR68C681_H

#include "mackerel.h"

// Exar XR68C681 / MC68681-family DUART register map. The DUART decodes on odd
// byte addresses; channel B is the console. UART_BASE is set per board in
// mackerel.h.
#define DUART1_MR1A (UART_BASE + 0x01)
#define DUART1_MR2A (UART_BASE + 0x01)
#define DUART1_SRA (UART_BASE + 0x03)
#define DUART1_CSRA (UART_BASE + 0x03)
#define DUART1_CRA (UART_BASE + 0x05)
#define DUART1_MISR (UART_BASE + 0x05)
#define DUART1_RBA (UART_BASE + 0x07)
#define DUART1_TBA (UART_BASE + 0x07)
#define DUART1_IPCR (UART_BASE + 0x09)
#define DUART1_ACR (UART_BASE + 0x09)
#define DUART1_ISR (UART_BASE + 0x0B)
#define DUART1_IMR (UART_BASE + 0x0B)
#define DUART1_CUR (UART_BASE + 0x0D)
#define DUART1_CLR (UART_BASE + 0x0F)
#define DUART1_MR1B (UART_BASE + 0x11)
#define DUART1_MR2B (UART_BASE + 0x11)
#define DUART1_SRB (UART_BASE + 0x13)
#define DUART1_CSRB (UART_BASE + 0x13)
#define DUART1_CRB (UART_BASE + 0x15)
#define DUART1_RBB (UART_BASE + 0x17)
#define DUART1_TBB (UART_BASE + 0x17)
#define DUART1_IVR (UART_BASE + 0x19)
#define DUART1_IP (UART_BASE + 0x1B)
#define DUART1_OPCR (UART_BASE + 0x1B)
#define DUART1_OPR (UART_BASE + 0x1D)
#define DUART1_OPR_RESET (UART_BASE + 0x1F)

// Interrupt bits
#define DUART_INTR_COUNTER 0b0001000
#define DUART_INTR_RXRDY 0b00100000

#endif
