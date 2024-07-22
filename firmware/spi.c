#include <stdbool.h>
#include "spi.h"
#include "mackerel.h"

void gpio_put(uint8_t pin, bool val)
{
    if (val)
    {
        MEM(DUART1_OPR_RESET) = (1 << pin);
    }
    else
    {
        MEM(DUART1_OPR) = (1 << pin);
    }
}

bool gpio_get(uint8_t pin)
{
    unsigned char in = MEM(DUART1_IP);
    unsigned char shift = (1 << pin);

    if (in & shift) {
        return true;
    }
    else {
        return false;
    }
}

void spi_init()
{
    // Default state of the SPI pins
    gpio_put(CS, true);
    gpio_put(MOSI, true);
    gpio_put(SCLK, false);

    gpio_put(LED, false);
}

void spi_clk(bool on){
    gpio_put(SCLK, on);
    gpio_put(LED, on);
    // delay(10);
}

uint8_t spi_transfer(uint8_t byte_to_send)
{
    uint8_t byte_received = 0;
    int i;

    spi_clk(false);

    // Set chip select (active low)
    gpio_put(CS, false);

    // delay(10);

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
        spi_clk(true);

        // Shift in a bit
        if (gpio_get(MISO))
        {
            byte_received |= (1 << i);
        }

        // Clock low
        spi_clk(false);
    }

    // disable SD card
    gpio_put(CS, true);

    return byte_received;
}
