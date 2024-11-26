#include "ide.h"
#include "mackerel.h"
#include <stdio.h>

// Reverse the order of the low 8-bits in a 16-bit number
uint16_t reverse_lowest_8_bits(uint16_t num)
{
    uint8_t low_bits = num & 0xFF;

    uint8_t reversed_low_bits = 0;
    for (int i = 0; i < 8; i++)
    {
        reversed_low_bits |= ((low_bits >> i) & 1) << (7 - i);
    }

    return (num & 0xFF00) | reversed_low_bits;
}

void read_sector(uint16_t *buf)
{
    for (uint16_t i = 0; i < 256; i++)
    {
        // IDE_wait_for_data_ready();   // needed at higher speeds?
        buf[i] = MEM16(IDE_DATA);

#ifdef MACKEREL_30
        // NOTE: Mackerel-30 v0.1 has the low byte of the IDE interface wired in reverse order
        buf[i] = reverse_lowest_8_bits(buf[i]);
#endif
    }
}

void IDE_wait_for_device_ready()
{
    while ((MEM(IDE_STATUS) & IDE_SR_DRDY) == 0)
    {
    }
}

void IDE_wait_for_data_ready()
{
    while ((MEM(IDE_STATUS) & IDE_SR_DRQ) == 0)
    {
    }
}

void IDE_read_sector(uint16_t *buf, uint32_t lba)
{
    // NOTE: IDE sector count starts at 1 (not true in LBA mode?)

    // NOTE: limited to 24 bit LBA addressing (~8GB)

    // Select master drive
    MEM(IDE_DRIVE_SEL) = 0xE0;

    MEM(IDE_SECTOR_COUNT) = 1;

    MEM(IDE_SECTOR_START) = (uint8_t)(lba & 0xFF);
    MEM(IDE_LBA_MID) = (uint8_t)((lba >> 8) & 0xFF);
    MEM(IDE_LBA_HIGH) = (uint8_t)((lba >> 16) & 0xFF);

    // Send the read sector command
    MEM(IDE_COMMAND) = IDE_CMD_READ_SECTOR;
    IDE_wait_for_data_ready();
    read_sector(buf);
}

void IDE_device_info(uint16_t *buf)
{
    printf("Reading IDE device info...\r\n");
    MEM(IDE_COMMAND) = IDE_CMD_IDENTIFY;
    IDE_wait_for_data_ready();
    read_sector(buf);

    // Model
    for (int i = 27; i <= 46; i++)
    {
        printf("%c%c", (char)((buf[i] & 0x00FF)), (char)((buf[i] & 0xFF00) >> 8));
    }

    printf("\r\n");

    // Version
    for (int i = 23; i <= 26; i++)
    {
        printf("%c%c", (char)((buf[i] & 0x00FF)), (char)((buf[i] & 0xFF00) >> 8));
    }

    printf("\r\n");

    // Serial number
    for (int i = 10; i <= 19; i++)
    {
        printf("%c%c", (char)((buf[i] & 0x00FF)), (char)((buf[i] & 0xFF00) >> 8));
    }

    printf("\r\n");
}
