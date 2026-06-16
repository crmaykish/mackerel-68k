#ifndef _SD_SPI_H
#define _SD_SPI_H

#include <stdbool.h>
#include "mackerel.h"

// SD card over the tiny_spi master (SPI mode). CS = GPIO bit 6.

// Bring the card up (CMD0/CMD8/ACMD41/CMD58); returns false on failure.
bool sd_spi_init(void);

// Read `count` 512-byte blocks from LBA `lba` into buf.
// Returns 0 on success, -1 on error (matches fat16_read_sector_f).
int sd_spi_read(uint32_t lba, uint8_t *buf, uint32_t count);

// Print CID/CSD identity + capacity (call after sd_spi_init).
void sd_spi_print_info(void);

#endif
