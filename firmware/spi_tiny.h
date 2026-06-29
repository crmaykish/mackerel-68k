#ifndef _SPI_TINY_H
#define _SPI_TINY_H

#include "mackerel.h"

// tiny_spi registers
#ifdef MACKEREL_10
#define SPI_RXDATA (SPI_BASE + 1)
#define SPI_TXDATA (SPI_BASE + 3)
#define SPI_STATUS (SPI_BASE + 5)
#define SPI_CONTROL (SPI_BASE + 7)
#define SPI_BAUD (SPI_BASE + 9)
#define SPI_NIC_CS (SPI_BASE + 11)
#else
// Mackerel-F
#define SPI_RXDATA (SPI_BASE + 0)
#define SPI_TXDATA (SPI_BASE + 4)
#define SPI_STATUS (SPI_BASE + 8)
#define SPI_CONTROL (SPI_BASE + 12)
#define SPI_BAUD (SPI_BASE + 16)
#endif

// STATUS bits
#define SPI_TXE 0x01 // engine idle (transfer complete)
#define SPI_TXR 0x02 // ready to accept the next byte

void spi_init(uint8_t baud);
uint8_t spi_transfer(uint8_t tx);

#endif
