#include "mackerel.h"

void __attribute__((interrupt)) MFPTimerBTick()
{
    MEM(MFP_GPDR) += 1;
}

int main()
{
    int a = 'A';
    
    // Map an exception handler for the MFP timer B interrupt
    set_exception_handler(0x48, &MFPTimerBTick);

    // Set MFP Timer B to run at 36 Hz and trigger an interrupt on every tick
    MEM(MFP_TBDR) = 0;         // Timer B counter max (i.e 255);
    MEM(MFP_TBCR) = 0b0010111; // Timer B enabled, delay mode, /200 prescalar
    MEM(MFP_VR) = 0x40;        // Set base vector for MFP interrupt handlers
    MEM(MFP_IERA) = 0x01;      // Enable interrupts for Timer B
    MEM(MFP_IMRA) = 0x01;      // Unmask interrupt for Timer B

    // Turn interrupts on
    set_interrupts(true);
    
    while (1)
    {
        mfp_putc(a);

        a++;

        if (a > 'Z')
        {
            a = 'A';
        }

        delay(1000);
    }

    return 0;
}
