#include <stdio.h>
#include <string.h>

#include "mackerel.h"
// #include "ch376s.h"



int main()
{
    printf("Hello from Mackerel. Here are some numbers %d %04X\r\n", 99, 0xBEEF);

    // usb_reset();

    // size_t file_size = file_read("APPLE2.TXT", (uint8_t *)0xC0000);

    // printf("File size: %ld\r\n%s", file_size, (char *)0xC0000);

    // uint8_t i = 0;

    // while (1)
    // {
    //     MEM(MFP_GPDR) = i;
    //     i++;
    //     delay(1000);
    // }

    return 0;
}
