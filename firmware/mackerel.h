#ifndef _MACKEREL_H
#define _MACKEREL_H

#define ACIA_DATA 0xC0000
#define ACIA_STATUS 0xC0001
#define ACIA_COMMAND 0xC0002
#define ACIA_CONTROL 0xC0003

#define ACIA_TX_READY 0x10
#define ACIA_RX_READY 0x08

// Get a pointer to a memory address
#define MEM(address) (*(volatile unsigned char *)(address))

// Hardware Setup
void m_init();

// Console I/O
void m_putc(char a);
void m_puts(const char *s);
char m_getc();
void m_readline(char *buffer);

int m_printf(const char *fmt, ...);

#endif
