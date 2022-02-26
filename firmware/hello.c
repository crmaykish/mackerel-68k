#include <stdio.h>
#include <string.h>

#include "mackerel.h"
// #include "ch376s.h"

void __attribute__((interrupt)) MFPTimerBTick()
{
    MEM(MFP_GPDR) += 1;
}

int main()
{
    mfp_puts("Starting test...\r\n");

    // Map an exception handler for the MFP timer B interrupt
    set_exception_handler(0x48, &MFPTimerBTick);

    // Set MFP Timer B to run at 36 Hz and trigger an interrupt on every tick
    MEM(MFP_TBDR) = 0;         // Timer B counter max (i.e 255);
    MEM(MFP_TBCR) = 0b0010111; // Timer B enabled, delay mode, /200 prescalar
    MEM(MFP_VR) = 0x40;        // Set base vector for MFP interrupt handlers
    MEM(MFP_IERA) = 0x01;      // Enable interrupts for Timer B
    MEM(MFP_IMRA) = 0x01;      // Unmask interrupt for Timer B

    int a = 'A';

    while (1)
    {
        mfp_puts("tick: ");
        mfp_putc(a);
        mfp_puts("\r\n");
        delay(10000);

        a++;

        if (a > 'Z')
        {
            a = 'A';
        }
    }

    // printf("Hello from Mackerel. Here are some numbers %d %04X\r\n", 99, 0xBEEF);

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
