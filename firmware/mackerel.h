#ifndef _MACKEREL_H
#define _MACKEREL_H

#include <stdbool.h>

// NOTE: Baselibc has no stdint header
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

// Memory

#define RAM_SIZE 0x180000
#define ROM_START 0x380000
#define VECTOR_TABLE_SIZE 0x400
#define INIT_SP_ADDRESS 0x8000

// CH376S USB Module

#define USB_DATA 0x3E8000
#define USB_COMMAND 0x3E8001

// MC68681P DUART
#define DUART_BASE 0x3C0000
#define DUART_MR1A (DUART_BASE + 0x01)
#define DUART_MR2A (DUART_BASE + 0x01)
#define DUART_SRA (DUART_BASE + 0x03)
#define DUART_CSRA (DUART_BASE + 0x03)
#define DUART_CRA (DUART_BASE + 0x05)
#define DUART_RBA (DUART_BASE + 0x07)
#define DUART_TBA (DUART_BASE + 0x07)
#define DUART_ACR (DUART_BASE + 0x09)
#define DUART_ISR (DUART_BASE + 0x0B)
#define DUART_IMR (DUART_BASE + 0x0B)
#define DUART_CUR (DUART_BASE + 0x0D)
#define DUART_CLR (DUART_BASE + 0x0F)
#define DUART_IVR (DUART_BASE + 0x19)
#define DUART_OPCR (DUART_BASE + 0x1B)
#define DUART_OPR (DUART_BASE + 0x1D)
#define DUART_OPR_RESET (DUART_BASE + 0x1F)

// Get the value at a memory address
#define MEM(address) (*(volatile unsigned char *)(address))

// Get the value starting at memory address as a uint
#define MEM_UINT(address) (*(volatile uint32_t *)(address))

// Enable/disable CPU interrupts
void set_interrupts(bool enabled);

// Assign a handler function to an exception number (0x00 - 0xFF)
void set_exception_handler(unsigned char exception_number, void (*exception_handler)());

// DUART
extern void duart_init();
void duart_putc(char c);
void duart_puts(const char *s);
char duart_getc();

// Timing
void delay(int time);

#endif
