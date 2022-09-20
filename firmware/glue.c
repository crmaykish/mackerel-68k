#include <stdio.h>
#include "mackerel.h"

// Glue code for Baselibc

size_t uart_write(FILE *instance, const char *bp, size_t n)
{
	for (size_t c = 0; c < n; c++)
	{
		mputc(bp[c]);
	}
	return n;
}

const struct File_methods output_methods = { uart_write, NULL };
struct File stdout_data = { &output_methods };
FILE* const stdout = &stdout_data;
