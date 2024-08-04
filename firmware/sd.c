#include <stdio.h>
#include "sd.h"
#include "spi.h"

static uint8_t CMD0[6] = {0x40 + 0, 0x00, 0x00, 0x00, 0x00, 0x95};
static uint8_t CMD8[6] = {0x40 + 8, 0x00, 0x00, 0x01, 0xAA, 0x87};
static uint8_t CMD58[6] = {0x40 + 58, 0x00, 0x00, 0x00, 0x00, 0x01};
static uint8_t CMD55[6] = {0x40 + 55, 0x00, 0x00, 0x00, 0x00, 0x01};
static uint8_t ACMD41[6] = {0x40 + 41, 0x40, 0x00, 0x00, 0x00, 0x01};
static uint8_t CMD17[6] = {0x40 + 17, 0x00, 0x00, 0x00, 0x00, 0x01};

static uint8_t sd_get_response();

bool sd_init()
{
    int i;
    uint8_t b;
    bool init_done = false;

    spi_init(CS_SD);

    delay(100);

    spi_loop_clk();

    b = spi_transfer(CS_SD, SPI_EMPTY);

    if (b != SPI_EMPTY)
    {
        printf("First SD byte read failed: %02X\r\n", b);
        return false;
    }

    // CMD0
    b = sd_command(CMD0);

    if (b != 0x01)
    {
        printf("CMD0 failed: %02X\r\n", b);
        return false;
    }

    // CMD8
    b = sd_command(CMD8);

    if (b != 0x01)
    {
        printf("CMD8 failed %02X\r\n", b);
        return false;
    }

    // Read the rest of the response and discard
    spi_transfer(CS_SD, SPI_EMPTY);
    spi_transfer(CS_SD, SPI_EMPTY);
    spi_transfer(CS_SD, SPI_EMPTY);
    spi_transfer(CS_SD, SPI_EMPTY);

    // CMD58
    b = sd_command(CMD58);

    if (b != 0x01)
    {
        printf("CMD58 failed %02X\r\n", b);
        return false;
    }

    // Read the rest of the response and discard
    spi_transfer(CS_SD, SPI_EMPTY);
    spi_transfer(CS_SD, SPI_EMPTY);
    spi_transfer(CS_SD, SPI_EMPTY);
    spi_transfer(CS_SD, SPI_EMPTY);

    i = 0;

    printf("Sending SD init command...\r\n");
    while (!init_done && i < SPI_RETRY_LIMIT)
    {

        b = sd_command(CMD55);

        if (b != 0x01)
        {
            printf("CMD55 failed %02X\r\n", b);
            return false;
        }

        b = sd_command(ACMD41);

        if (b == 0x00)
        {
            init_done = true;
        }

        i++;
    }

    if (init_done)
    {
        printf("SD card ready!\r\n");
    }
    else
    {
        printf("Failed to initialize SD card\r\n");
    }

    return init_done;
}

uint8_t sd_command(uint8_t command[6])
{
    int i;

    for (i = 0; i < 6; i++)
    {
        spi_transfer(CS_SD, command[i]);
    }

    return sd_get_response();
}

void sd_read(uint32_t block_num, uint8_t *block)
{
    int i;
    uint8_t result;
    uint8_t command[6] = {0x40 + 17, 0, 0, 0, 0, 0x01};

    if (!block)
    {
        printf("block pointer cannot be NULL\r\n");
        return;
    }

    command[4] = (block_num & 0xFF);
    command[3] = ((block_num & 0xFF00) >> 8);

    result = sd_command(command);

    if (result != 0x00)
    {
        printf("CMD17 failed\r\n");
        return;
    }

    while (result != 0xFE)
    {
        result = sd_get_response();
    }

    for (i = 0; i < 512; i++)
    {
        block[i] = spi_transfer(CS_SD, SPI_EMPTY);
    }

    // read the 16 bit CRC and ignore
    spi_transfer(CS_SD, SPI_EMPTY);
    spi_transfer(CS_SD, SPI_EMPTY);
}

static uint8_t sd_get_response()
{
    uint8_t response = spi_transfer(CS_SD, SPI_EMPTY);
    int count = 0;

    while (response == 0xFF && count < SPI_RETRY_LIMIT)
    {
        response = spi_transfer(CS_SD, SPI_EMPTY);
        count++;
    }

    return response;
}
