#include "mackerel.h"

// Glue functions required to use the newlib standard library

void write(int file, char *s, int len)
{
    for (int i = 0; i < len; i++)
    {
        mfp_putc(s[i]);
    }
}

void sbrk() {}
void close() {}
void fstat() {}
void isatty() {}
void lseek() {}
void read() {}
