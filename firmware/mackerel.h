#ifndef _MACKEREL_H
#define _MACKEREL_H

#include <stdbool.h>

// NOTE: Baselibc has no stdint header
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

// Memory

// MC68681P DUART 1

#define DUART1_BASE 0xC00000
// #define DUART1_BASE 0x3FC000
#define DUART1_MR1A (DUART1_BASE + 0x01)
#define DUART1_MR2A (DUART1_BASE + 0x01)
#define DUART1_SRA (DUART1_BASE + 0x03)
#define DUART1_CSRA (DUART1_BASE + 0x03)
#define DUART1_CRA (DUART1_BASE + 0x05)
#define DUART1_MISR (DUART1_BASE + 0x05)
#define DUART1_RBA (DUART1_BASE + 0x07)
#define DUART1_TBA (DUART1_BASE + 0x07)
#define DUART1_IPCR (DUART1_BASE + 0x09)
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
#define DUART1_IP (DUART1_BASE + 0x1B)
#define DUART1_OPCR (DUART1_BASE + 0x1B)
#define DUART1_OPR (DUART1_BASE + 0x1D)
#define DUART1_OPR_RESET (DUART1_BASE + 0x1F)


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

void set_leds(unsigned char val);

void set_gpio(unsigned char val);

#endif
