#ifndef SPI_H
#define SPI_H

#include "mackerel.h"

// SPI output pins
#define SCLK (3)
#define MOSI (2) // DI on SD card
#define CS0 (4)
#define CS1 (5)
#define CS2 (6)

// SPI input pins
#define MISO (4) // DO on SD card

#define LED (7)

#define SPI_EMPTY 0xFF

#define SPI_RETRY_LIMIT 10

// Set up the GPIO pins needed for SPI
void spi_init(uint8_t cs);

// Send and receive a single byte over SPI
uint8_t spi_transfer(uint8_t cs, uint8_t b);

void spi_loop_clk();

#endif
