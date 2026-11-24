// Mackerel-F: write a kernel image to the SD card's FAT16 partition as IMAGE.BIN
#ifndef _SDFLASH_H
#define _SDFLASH_H

#include <stdbool.h>
#include <stdint.h>

// Bytes of scratch RAM sdflash_write() needs (sector buffer + read-back chunks)
#define SDFLASH_SCRATCH (32 * 512)

// Read and sanity-check the FAT16 boot sector at fat16_part_start().
// Needs an initialised card and fat16_init(). Returns false if it isn't usable.
bool sdflash_prepare(void);

// Rewrite the partition so IMAGE.BIN (img, len bytes) is its only file, stored in
// contiguous clusters, then read it back and compare. Call sdflash_prepare() first.
// scratch must be SDFLASH_SCRATCH bytes that don't overlap img.
bool sdflash_write(const uint8_t *img, uint32_t len, uint8_t *scratch);

#endif
