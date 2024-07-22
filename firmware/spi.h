#ifndef SPI_H
#define SPI_H

#include "mackerel.h"

// SPI output pins
#define SCLK (3)
#define MOSI (2) // DI on SD card
#define CS (7)

// SPI input pins
#define MISO (5) // DO on SD card
#define CD (2)   // card detect

#define LED (6)

#define SPI_EMPTY 0xFF

#define SPI_RETRY_LIMIT 10

// GPIO control
void gpio_put(uint8_t pin, bool val);
bool gpio_get(uint8_t pin);

// Set up the GPIO pins needed for SPI
void spi_init();

void spi_clk(bool on);

// Send and receive a single byte over SPI
uint8_t spi_transfer(uint8_t b);

#endif