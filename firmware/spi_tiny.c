#include "spi_tiny.h"

void spi_init(uint8_t baud)
{
    MEM(SPI_BAUD) = baud;
}

// Single-byte full-duplex transfer, matching the Linux spi-oc-tiny len==1 path:
// write TXDATA to start, wait for TXE (engine idle), read the received byte.
uint8_t spi_transfer(uint8_t tx)
{
    MEM(SPI_TXDATA) = tx;
    while (!(MEM(SPI_STATUS) & SPI_TXE)) {}
    return MEM(SPI_RXDATA);
}
