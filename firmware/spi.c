#include <stdbool.h>
#include "spi.h"
#include "mackerel.h"

#define GPIO_ON(pin) MEM(DUART1_OPR_RESET) = (1 << pin)
#define GPIO_OFF(pin) MEM(DUART1_OPR) = (1 << pin);
#define MISO_GET() (MEM(DUART1_IP) & (1 << MISO))

void spi_init(uint8_t cs)
{
    GPIO_ON(cs);
    GPIO_ON(MOSI);
    GPIO_OFF(SCLK);
    GPIO_OFF(LED);
}

void spi_cs_low(uint8_t cs)
{
    GPIO_OFF(cs);
}

void spi_cs_high(uint8_t cs)
{
    GPIO_ON(cs);
}

uint8_t spi_byte(uint8_t b)
{
    uint8_t recv = 0;
    int i;

    GPIO_OFF(SCLK);

    for (i = 7; i >= 0; i--)
    {
        if (b & (1 << i))
            GPIO_ON(MOSI);
        else
            GPIO_OFF(MOSI);

        GPIO_ON(SCLK);
        if (MISO_GET())
            recv |= (1 << i);
        GPIO_OFF(SCLK);
    }

    return recv;
}

// Portable byte receive. Mackerel-08 uses the faster inline-asm version in
// spi.h instead.
#ifndef MACKEREL_08
uint8_t spi_recv(void)
{
    uint8_t b = 0;
    GPIO_ON(SCLK); if (MISO_GET()) b |= 0x80; GPIO_OFF(SCLK);
    GPIO_ON(SCLK); if (MISO_GET()) b |= 0x40; GPIO_OFF(SCLK);
    GPIO_ON(SCLK); if (MISO_GET()) b |= 0x20; GPIO_OFF(SCLK);
    GPIO_ON(SCLK); if (MISO_GET()) b |= 0x10; GPIO_OFF(SCLK);
    GPIO_ON(SCLK); if (MISO_GET()) b |= 0x08; GPIO_OFF(SCLK);
    GPIO_ON(SCLK); if (MISO_GET()) b |= 0x04; GPIO_OFF(SCLK);
    GPIO_ON(SCLK); if (MISO_GET()) b |= 0x02; GPIO_OFF(SCLK);
    GPIO_ON(SCLK); if (MISO_GET()) b |= 0x01; GPIO_OFF(SCLK);
    return b;
}
#endif

uint8_t spi_transfer(uint8_t cs, uint8_t b)
{
    spi_cs_low(cs);
    uint8_t recv = spi_byte(b);
    spi_cs_high(cs);
    return recv;
}

void spi_loop_clk()
{
    // Toggle the SPI clock 80 times with CS disabled
    for (int i = 0; i < 80; i++)
    {
        GPIO_ON(SCLK);
        GPIO_OFF(SCLK);
    }
}
