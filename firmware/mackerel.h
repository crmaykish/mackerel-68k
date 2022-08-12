#ifndef _MACKEREL_H
#define _MACKEREL_H

#include <stdbool.h>

// NOTE: Baselibc has no stdint header
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

// Memory

#define RAM_SIZE 0x200000
#define ROM_START 0x3F8000
#define VECTOR_TABLE_SIZE 0x400
#define INIT_SP_ADDRESS 0x3F8000

// CH376S USB Module

#define USB_DATA 0x3E8000
#define USB_COMMAND 0x3E8001

// MC68901 Multi-function Peripheral

#define MFP_GPDR 0x3F0001
#define MFP_DDR 0x3F0005
#define MFP_IERA 0x3F0007
#define MFP_IERB 0x3F0009
#define MFP_IMRA 0x3F0013
#define MFP_IMRB 0x3F0015
#define MFP_TACR 0x3F0019
#define MFP_TBCR 0x3F001B
#define MFP_TCDCR 0x3F001D
#define MFP_TADR 0x3F001F
#define MFP_TBDR 0x3F0021
#define MFP_TCDR 0x3F0023
#define MFP_TDDR 0x3F0015
#define MFP_VR 0x3F0017  // Vector Register
#define MFP_UCR 0x3F0029 // USART Control Register
#define MFP_RSR 0x3F002B // USART Receiver Status Register
#define MFP_TSR 0x3F002D // USART Transmitter Status Register
#define MFP_UDR 0x3F002F // USART Data Register

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
