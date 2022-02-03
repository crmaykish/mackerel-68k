#include <stdio.h>
#include <stdbool.h>

#include "mackerel.h"

int main()
{
    printf("Starting kernel...\r\n");
    while (true)
    {
        delay(10000);
        printf("kernel tick\r\n");
    }

    return 0;
}
