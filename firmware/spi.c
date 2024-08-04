#include <stdbool.h>
#include "spi.h"
#include "mackerel.h"

static inline void gpio_put(uint8_t pin, bool val)
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

static inline bool gpio_get(uint8_t pin)
{
    unsigned char in = MEM(DUART1_IP);
    unsigned char shift = (1 << pin);
    return (in & shift);
}

void spi_init(uint8_t cs)
{
    // Default state of the SPI pins
    gpio_put(cs, true);
    gpio_put(MOSI, true);
    gpio_put(SCLK, false);

    gpio_put(LED, false);
}

static inline void spi_clk(bool on){
    gpio_put(SCLK, on);
}

uint8_t spi_transfer(uint8_t cs, uint8_t byte_to_send)
{
    uint8_t byte_received = 0;
    int i;

    spi_clk(false);

    // Set chip select (active low)
    gpio_put(cs, false);

    // delay(10);

    for (i = 7; i >= 0; i--)
    {
        gpio_put(MOSI, byte_to_send & (1 << i));

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
    gpio_put(cs, true);

    return byte_received;
}

void spi_loop_clk() {
    // Toggle the SPI clock 80 times with CS disabled
    for (int i = 0; i < 80; i++)
    {
        spi_clk(true);
        spi_clk(false);
    }
}
