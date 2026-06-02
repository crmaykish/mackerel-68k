#include <stdio.h>
#include <string.h>
#include "sd.h"
#include "spi.h"
#include "mackerel.h"
#include "term.h"

static uint8_t CMD0[6]   = {0x40 +  0, 0x00, 0x00, 0x00, 0x00, 0x95};
static uint8_t CMD8[6]   = {0x40 +  8, 0x00, 0x00, 0x01, 0xAA, 0x87};
static uint8_t CMD12[6]  = {0x40 + 12, 0x00, 0x00, 0x00, 0x00, 0x01};
static uint8_t CMD17[6]  = {0x40 + 17, 0x00, 0x00, 0x00, 0x00, 0x01};
static uint8_t CMD18[6]  = {0x40 + 18, 0x00, 0x00, 0x00, 0x00, 0x01};
static uint8_t CMD55[6]  = {0x40 + 55, 0x00, 0x00, 0x00, 0x00, 0x01};
static uint8_t CMD58[6]  = {0x40 + 58, 0x00, 0x00, 0x00, 0x00, 0x01};
static uint8_t ACMD41[6] = {0x40 + 41, 0x40, 0x00, 0x00, 0x00, 0x01};

static uint8_t sd_get_response();

// Send a command with CS already asserted and return the R1 response.
static uint8_t sd_send_command(uint8_t command[6])
{
    for (int i = 0; i < 6; i++)
        spi_byte(command[i]);

    uint8_t r;
    int count = 0;
    do {
        r = spi_byte(SPI_EMPTY);
        count++;
    } while (r == 0xFF && count < SPI_RETRY_LIMIT);
    return r;
}

bool sd_reset()
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

    spi_transfer(CS_SD, SPI_EMPTY);
    spi_transfer(CS_SD, SPI_EMPTY);
    spi_transfer(CS_SD, SPI_EMPTY);
    spi_transfer(CS_SD, SPI_EMPTY);

    i = 0;

    printf("Sending SD init command...\r\n");
    while (!init_done && i < SD_INIT_RETRY_LIMIT)
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
        else
        {
            // Card still busy; give it real time before polling again.
            delay(1000);
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

bool sd_init()
{
    bool sd_ready = false;

    for (int i = 0; i < 5; i++)
    {
        sd_ready = sd_reset();

        if (sd_ready)
            break;
    }

    return sd_ready;
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
    if (!block)
    {
        printf("block pointer cannot be NULL\r\n");
        return;
    }

    uint8_t command[6];
    memcpy(command, CMD17, 6);
    command[1] = (block_num >> 24) & 0xFF;
    command[2] = (block_num >> 16) & 0xFF;
    command[3] = (block_num >> 8)  & 0xFF;
    command[4] =  block_num        & 0xFF;

    spi_cs_low(CS_SD);

    uint8_t result = sd_send_command(command);

    if (result != 0x00)
    {
        printf("CMD17 failed\r\n");
        spi_cs_high(CS_SD);
        return;
    }

    int timeout = 8192;
    do {
        result = spi_byte(SPI_EMPTY);
    } while (result != 0xFE && --timeout > 0);

    if (result != 0xFE)
    {
        printf("No data token\r\n");
        spi_cs_high(CS_SD);
        return;
    }

    for (int i = 0; i < 512; i++)
        block[i] = spi_recv();

    spi_recv();
    spi_recv();

    spi_cs_high(CS_SD);
}

bool sd_read_blocks(uint32_t start_block, uint32_t num_blocks, uint8_t *buf)
{
    uint8_t command[6];
    memcpy(command, CMD18, 6);
    command[1] = (start_block >> 24) & 0xFF;
    command[2] = (start_block >> 16) & 0xFF;
    command[3] = (start_block >> 8)  & 0xFF;
    command[4] =  start_block        & 0xFF;

    spi_cs_low(CS_SD);

    uint8_t result = sd_send_command(command);

    if (result != 0x00)
    {
        printf("CMD18 failed: %02X\r\n", result);
        spi_cs_high(CS_SD);
        return false;
    }

    // Progress reporting: split the transfer into ~50 updates (2% each).
    // Single up-front division; the loop only adds and compares.
    uint32_t pct_step = num_blocks / 50;
    if (pct_step == 0)
        pct_step = 1;
    uint32_t next_update = pct_step;
    int pct = 0;

    // Hide the cursor while the progress bar redraws to keep it from
    // flickering across the bar. Re-enabled on every exit path below.
    term_cursor_set_vis(false);

    for (uint32_t block = 0; block < num_blocks; block++)
    {
        int timeout = 8192;
        do {
            result = spi_byte(SPI_EMPTY);
        } while (result != 0xFE && --timeout > 0);

        if (result != 0xFE)
        {
            term_cursor_set_vis(true);
            printf("Block %lu: no data token\r\n", block);
            for (int i = 0; i < 6; i++)
                spi_byte(CMD12[i]);
            spi_cs_high(CS_SD);
            return false;
        }

        for (int i = 0; i < 512; i++)
            *buf++ = spi_recv();

        spi_recv();
        spi_recv();

        // Progress indicator: redraw the percentage only when we cross a step.
        if (block + 1 >= next_update)
        {
            next_update += pct_step;
            pct += 2;
            if (pct > 100)
                pct = 100;
            term_progress_bar(pct);
        }
    }
    term_progress_bar(100);
    duart_putc('\n');
    term_cursor_set_vis(true);

    // CMD12: stop transmission
    for (int i = 0; i < 6; i++)
        spi_byte(CMD12[i]);

    // Discard stuff byte then poll R1
    spi_byte(SPI_EMPTY);
    int count = 0;
    do {
        result = spi_byte(SPI_EMPTY);
        count++;
    } while (result == 0xFF && count < SPI_RETRY_LIMIT);

    spi_cs_high(CS_SD);
    return true;
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
