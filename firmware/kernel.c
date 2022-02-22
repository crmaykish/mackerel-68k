#include <stdio.h>
#include <stdbool.h>

#include "mackerel.h"

int main()
{
    int i = 0;

    printf("Starting kernel...\r\n");

    while (true)
    {
        delay(10000);
        printf("kernel tick %d\r\n", i);
        i++;
    }

    return 0;
}
