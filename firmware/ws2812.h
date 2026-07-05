#ifndef _WS2812_H
#define _WS2812_H

#include "mackerel.h"

// Onboard WS2812B RGB LED (Mackerel-F only)
#define WS2812_G (WS2812_BASE + 0)  // write green, read STATUS
#define WS2812_R (WS2812_BASE + 2)  // write red
#define WS2812_B (WS2812_BASE + 4)  // write blue
#define WS2812_BUSY 0x01

// Set the LED to (r,g,b)
// Waits for WS2812 controller to show not busy
static inline void ws2812_set(uint8_t r, uint8_t g, uint8_t b)
{
    while (MEM(WS2812_G) & WS2812_BUSY)
        ;
    MEM(WS2812_G) = g;
    MEM(WS2812_R) = r;
    MEM(WS2812_B) = b; // starts the transmit
}

#endif
