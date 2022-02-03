#ifndef _MACKEREL_H
#define _MACKEREL_H

// 65C51 ACIA

#define ACIA_DATA 0x10000
#define ACIA_STATUS 0x10001
#define ACIA_COMMAND 0x10002
#define ACIA_CONTROL 0x10003

#define ACIA_TX_READY 0x10
#define ACIA_RX_READY 0x08

// CH376S USB Module

#define USB_DATA 0x14000
#define USB_COMMAND 0x14001

// MC68901 Multi-function Peripheral

#define MFP_GPDR 0x18001
#define MFP_DDR 0x18005
#define MFP_TACR 0x18019
#define MFP_TBCR 0x1801B
#define MFP_TCDCR 0x1801D
#define MFP_TADR 0x1801F
#define MFP_TBDR 0x18021
#define MFP_TCDR 0x18023
#define MFP_TDDR 0x18015
#define MFP_UCR 0x18029 // USART Control Register
#define MFP_RSR 0x1802B // USART Receiver Status Register
#define MFP_TSR 0x1802D // USART Transmitter Status Register
#define MFP_UDR 0x1802F // USART Data Register

// Get a pointer to a memory address
#define MEM(address) (*(volatile unsigned char *)(address))

// Serial port
void serial_init();
void serial_putc(char a);
void serial_puts(const char *s);
char serial_getc();

// MFP
void mfp_init();
void mfp_putc(char s);
void mfp_puts(const char *s);
char mfp_getc();

void delay(int time);

#endif
