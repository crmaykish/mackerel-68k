#ifndef _MACKEREL_H
#define _MACKEREL_H

#include <stdbool.h>

// NOTE: Baselibc has no stdint header
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define EXCEPTION_AUTOVECTOR 24
#define EXCEPTION_USER 64

#define IRQ_NUM_IDE 3
#define IRQ_NUM_DUART 5
#define IRQ_NUM_TIMER 6

#ifdef MACKEREL_30
#define SYSTEM_NAME "Mackerel-30"
#define DUART1_BASE 0xF0000000
#define IDE_BASE 0xF0010000
#elif MACKEREL_10
#define SYSTEM_NAME "Mackerel-10"
#define DUART1_BASE 0xFF8000
#define IDE_BASE 0xFFC000
#elif MACKEREL_08
#define SYSTEM_NAME "Mackerel-08"
#define DUART1_BASE 0x3FC000
// IDE is not supported on Mackerel-08
#define IDE_BASE 0xFFFFFF
#else
#define SYSTEM_NAME "Mackerel"
#endif

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
#define MEM(address) (*(volatile uint8_t *)(address))

#define MEM16(address) (*(volatile uint16_t *)(address))

// Get the value starting at memory address as a uint
#define MEM32(address) (*(volatile uint32_t *)(address))

// Enable/disable CPU interrupts
void set_interrupts(bool enabled);

// Assign a handler function to an exception number (0x00 - 0xFF)
void set_exception_handler(unsigned char exception_number, void (*exception_handler)());

// DUART
void duart_init(void);
void duart_putc(char c);
void duart_puts(const char *s);
char duart_getc(void);

// Utils
void memdump(uint32_t address, uint32_t bytes);

uint16_t bswap16(uint16_t value);
uint32_t bswap32(uint32_t value);

// Timing
void delay(int time);

#ifdef MACKEREL_30
void set_vbr(unsigned int vbr_val);
unsigned int get_vbr();
#endif

#endif
