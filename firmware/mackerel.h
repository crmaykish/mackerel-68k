#ifndef _MACKEREL_H
#define _MACKEREL_H

#include <stdbool.h>

// NOTE: Baselibc has no stdint header
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

// Memory

#define RAM_SIZE 0x80000
#define ROM_START 0x380000
#define VECTOR_TABLE_SIZE 0x400
#define INIT_SP_ADDRESS 0x8000

// CH376S USB Module

#define USB_DATA 0x3E8000
#define USB_COMMAND 0x3E8001

// MC68901 Multi-function Peripheral

#define MFP_BASE 0x3C0000
#define MFP_GPDR (MFP_BASE + 0x01)
#define MFP_DDR (MFP_BASE + 0x05)
#define MFP_IERA (MFP_BASE + 0x07)
#define MFP_IERB (MFP_BASE + 0x09)
#define MFP_IMRA (MFP_BASE + 0x13)
#define MFP_IMRB (MFP_BASE + 0x15)
#define MFP_TACR (MFP_BASE + 0x19)
#define MFP_TBCR (MFP_BASE + 0x1B)
#define MFP_TCDCR (MFP_BASE + 0x1D)
#define MFP_TADR (MFP_BASE + 0x1F)
#define MFP_TBDR (MFP_BASE + 0x21)
#define MFP_TCDR (MFP_BASE + 0x23)
#define MFP_TDDR (MFP_BASE + 0x15)
#define MFP_VR (MFP_BASE + 0x17)  // Vector Register
#define MFP_UCR (MFP_BASE + 0x29) // USART Control Register
#define MFP_RSR (MFP_BASE + 0x2B) // USART Receiver Status Register
#define MFP_TSR (MFP_BASE + 0x2D) // USART Transmitter Status Register
#define MFP_UDR (MFP_BASE + 0x2F) // USART Data Register

// MC68691 DUART

#define DUART_MR1A 0x3E0001
#define DUART_MR2A 0x3E0001
#define DUART_SRA 0x3E0003
#define DUART_CSRA 0x3E0003
#define DUART_CRA 0x3E0005
#define DUART_RBA 0x3E0007
#define DUART_TBA 0x3E0007
#define DUART_ACR 0x3E0009
#define DUART_ISR 0x3E000B
#define DUART_IMR 0x3E000B
#define DUART_CUR 0x3E000D
#define DUART_CLR 0x3E000F
#define DUART_OPCR 0x3E001B
#define DUART_OPR 0x3E001D
#define DUART_OPR_RESET 0x3E001F

// Get the value at a memory address
#define MEM(address) (*(volatile unsigned char *)(address))

// Get the value starting at memory address as a uint
#define MEM_UINT(address) (*(volatile uint32_t *)(address))

// Enable/disable CPU interrupts
void set_interrupts(bool enabled);

// Assign a handler function to an exception number (0x00 - 0xFF)
void set_exception_handler(unsigned char exception_number, void (*exception_handler)());

// MFP
void mfp_init();
void mfp_putc(char s);
void mfp_puts(const char *s);
char mfp_getc();

// DUART
extern void duart_init();
void duart_putc(char c);
void duart_puts(const char *s);
char duart_getc();

void delay(int time);

#endif
