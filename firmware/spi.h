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

// Set up the GPIO pins needed for SPI
void spi_init();

// Send and receive a single byte over SPI
uint8_t spi_transfer(uint8_t b);

void spi_loop_clk();

bool card_detect();

#endif
