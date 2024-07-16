#ifndef _MACKEREL_H
#define _MACKEREL_H

#include <stdbool.h>

#define mputc(s) duart_putc(s)
#define mputs(s) duart_puts(s)
#define mgetc() duart_getc()

// NOTE: Baselibc has no stdint header
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

// Memory

// CH376S USB Module

#define USB_DATA 0x3E8000    // TODO
#define USB_COMMAND 0x3E8001 // TODO

// MC68681P DUART 1

#define DUART1_BASE 0x3C0000
#define DUART1_MR1A (DUART1_BASE + 0x01)
#define DUART1_MR2A (DUART1_BASE + 0x01)
#define DUART1_SRA (DUART1_BASE + 0x03)
#define DUART1_CSRA (DUART1_BASE + 0x03)
#define DUART1_CRA (DUART1_BASE + 0x05)
#define DUART1_MISR (DUART1_BASE + 0x05)
#define DUART1_RBA (DUART1_BASE + 0x07)
#define DUART1_TBA (DUART1_BASE + 0x07)
#define DUART1_ACR (DUART1_BASE + 0x09)
#define DUART1_ISR (DUART1_BASE + 0x0B)
#define DUART1_IMR (DUART1_BASE + 0x0B)
#define DUART1_CUR (DUART1_BASE + 0x0D)
#define DUART1_CLR (DUART1_BASE + 0x0F)
#define DUART1_MR1B (DUART1_BASE + 0x11)
#define DUART1_MR2B (DUART1_BASE + 0x11)
#define DUART1_SRB (DUART1_BASE + 0x13)
#define DUART1_CSRB (DUART1_BASE + 0x13)
#define DUART1_CRB (DUART1_BASE + 0x15)
#define DUART1_RBB (DUART1_BASE + 0x17)
#define DUART1_TBB (DUART1_BASE + 0x17)
#define DUART1_IVR (DUART1_BASE + 0x19)
#define DUART1_OPCR (DUART1_BASE + 0x1B)
#define DUART1_OPR (DUART1_BASE + 0x1D)
#define DUART1_OPR_RESET (DUART1_BASE + 0x1F)

// MC68681P DUART 2
#define DUART2_BASE 0x3C4000
#define DUART2_MR1A (DUART2_BASE + 0x01)
#define DUART2_MR2A (DUART2_BASE + 0x01)
#define DUART2_SRA (DUART2_BASE + 0x03)
#define DUART2_CSRA (DUART2_BASE + 0x03)
#define DUART2_CRA (DUART2_BASE + 0x05)
#define DUART2_MISR (DUART2_BASE + 0x05)
#define DUART2_RBA (DUART2_BASE + 0x07)
#define DUART2_TBA (DUART2_BASE + 0x07)
#define DUART2_ACR (DUART2_BASE + 0x09)
#define DUART2_ISR (DUART2_BASE + 0x0B)
#define DUART2_IMR (DUART2_BASE + 0x0B)
#define DUART2_CUR (DUART2_BASE + 0x0D)
#define DUART2_CLR (DUART2_BASE + 0x0F)
#define DUART2_MR1B (DUART2_BASE + 0x11)
#define DUART2_MR2B (DUART2_BASE + 0x11)
#define DUART2_SRB (DUART2_BASE + 0x13)
#define DUART2_CSRB (DUART2_BASE + 0x13)
#define DUART2_CRB (DUART2_BASE + 0x15)
#define DUART2_RBB (DUART2_BASE + 0x17)
#define DUART2_TBB (DUART2_BASE + 0x17)
#define DUART2_IVR (DUART2_BASE + 0x19)
#define DUART2_OPCR (DUART2_BASE + 0x1B)
#define DUART2_OPR (DUART2_BASE + 0x1D)
#define DUART2_OPR_RESET (DUART2_BASE + 0x1F)

// Interrupt bits
#define DUART_INTR_COUNTER 0b0001000
#define DUART_INTR_RXRDY 0b00100000

// Get the value at a memory address
#define MEM(address) (*(volatile unsigned char *)(address))

// Get the value starting at memory address as a uint
#define MEM_UINT(address) (*(volatile uint32_t *)(address))

// Enable/disable CPU interrupts
void set_interrupts(bool enabled);

// Assign a handler function to an exception number (0x00 - 0xFF)
void set_exception_handler(unsigned char exception_number, void (*exception_handler)());

// DUART
void duart_init(void);
void duart_putc(char c);
void duart_puts(const char *s);
char duart_getc(void);

// Timing
void delay(int time);

#endif
