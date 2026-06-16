#ifndef _CONSOLE_H
#define _CONSOLE_H

void console_init(void);
void console_putc(char c);
char console_getc(void);
void console_puts(const char *s);

#endif
