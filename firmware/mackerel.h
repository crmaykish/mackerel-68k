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

#define MFP_GPDR 0x18000
#define MFP_DDR 0x18002

// Get a pointer to a memory address
#define MEM(address) (*(volatile unsigned char *)(address))

// Hardware Setup
void serial_init();

// Serial port
void serial_putc(char a);
void serial_puts(const char *s);
char serial_getc();
void serial_readline(char *buffer);

#endif
