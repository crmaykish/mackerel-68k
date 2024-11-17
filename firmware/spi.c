#include <stdbool.h>
#include "spi.h"
#include "mackerel.h"

#define GPIO_ON(pin) MEM(DUART1_OPR_RESET) = (1 << pin)
#define GPIO_OFF(pin) MEM(DUART1_OPR) = (1 << pin);
#define MISO_GET() (MEM(DUART1_IP) & (1 << MISO))

void spi_init(uint8_t cs)
{
    // Default state of the SPI pins
    GPIO_ON(cs);
    GPIO_ON(MOSI);
    GPIO_OFF(SCLK);

    GPIO_OFF(LED);
}

uint8_t spi_transfer(uint8_t cs, uint8_t byte_to_send)
{
    uint8_t byte_received = 0;
    int i;

    GPIO_OFF(SCLK);

    // Set chip select (active low)
    GPIO_OFF(cs);

    for (i = 7; i >= 0; i--)
    {
        if (byte_to_send & (1 << i))
        {
            GPIO_ON(MOSI);
        }
        else
        {
            GPIO_OFF(MOSI);
        }

        // Clock high
        GPIO_ON(SCLK);

        // Shift in a bit
        if (MISO_GET())
        {
            byte_received |= (1 << i);
        }

        // Clock low
        GPIO_OFF(SCLK);
    }

    // disable SD card
    GPIO_ON(cs);

    return byte_received;
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
