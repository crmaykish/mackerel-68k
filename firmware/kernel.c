#include <stdio.h>
#include <stdbool.h>

#include "mackerel.h"

int main()
{
    int i = 0;

    printf("Starting kernel...%X\r\n", 0xDEADBEEF);

    while (true)
    {
        delay(1000);
        MEM(MFP_GPDR) = (unsigned char)(i & 0xFF);
        printf("kernel tick %d\r\n", i);
        i++;
    }

    return 0;
}
