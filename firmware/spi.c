#include <stdbool.h>
#include "spi.h"

void gpio_put(uint8_t pin, bool val)
{
    if (val)
    {
        MEM(SPI_IO_PORT) |= (1 << pin);
    }
    else
    {
        MEM(SPI_IO_PORT) &= ~(1 << pin);
    }
}

bool gpio_get(uint8_t pin)
{
    return ((MEM(SPI_IO_PORT) & (1 << pin)) > 0);
}

void spi_init()
{
    // Set MISO to input, other pins to output
    MEM(SPI_IO_DDR) = 0b11111101;

    // Default state of the SPI pins
    gpio_put(CS, true);
    gpio_put(MOSI, true);
    gpio_put(SCLK, false);
}

uint8_t spi_transfer(uint8_t byte_to_send)
{
    uint8_t byte_received = 0;
    int i;

    gpio_put(SCLK, false);

    // Set chip select (active low)
    gpio_put(CS, false);

    for (i = 7; i >= 0; i--)
    {
        // Shift out a bit
        if (byte_to_send & (1 << i))
        {
            gpio_put(MOSI, true);
        }
        else
        {
            gpio_put(MOSI, false);
        }

        // Clock high
        // gpio_put(SCLK, true);
        spi_clock_on();

        // Shift in a bit
        if (gpio_get(MISO))
        {
            byte_received |= (1 << i);
        }

        // Clock low
        gpio_put(SCLK, false);
    }

    // disable SD card
    gpio_put(CS, true);

    return byte_received;
}
