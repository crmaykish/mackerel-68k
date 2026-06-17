#ifndef _SD_SPI_H
#define _SD_SPI_H

#include <stdbool.h>
#include "mackerel.h"

// SD card over the tiny_spi master (SPI mode). CS = GPIO bit 6.

// SPI divisors: SCLK = clk_soc / (2*(baud+1)), clk_soc = 64.8 MHz.
#define SD_BAUD_INIT 128 // ~251 kHz (<400 kHz, for card init)
#define SD_BAUD_DATA 3   // ~8.1 MHz (data rate; HW sweep showed >8 MHz is CPU-bound)

// Bring the card up (CMD0/CMD8/ACMD41/CMD58); returns false on failure.
bool sd_spi_init(void);

// Set the SPI data-rate divisor (see SD_BAUD_* above).
void sd_spi_set_baud(uint8_t baud);

// Read `count` 512-byte blocks from LBA `lba` into buf.
// Returns 0 on success, -1 on error (matches fat16_read_sector_f).
int sd_spi_read(uint32_t lba, uint8_t *buf, uint32_t count);

// Print CID/CSD identity + capacity (call after sd_spi_init).
void sd_spi_print_info(void);

#endif
