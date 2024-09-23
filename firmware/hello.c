#include "mackerel.h"
#include <stdio.h>
#include <string.h>

// IDE mem-mapped registers
#define IDE_BASE 0x100000
#define IDE_DATA IDE_BASE + 0x00
// #define IDE_ERROR IDE_BASE + 0x03
// #define IDE_FEATURE IDE_BASE + 0x03
#define IDE_SECTOR_COUNT IDE_BASE + 0x05
#define IDE_SECTOR_START IDE_BASE + 0x07
#define IDE_STATUS IDE_BASE + 0x0F
#define IDE_COMMAND IDE_BASE + 0x0F

// IDE commands
#define IDE_CMD_RESET 0x08
#define IDE_CMD_READ_SECTOR 0x20
#define IDE_CMD_IDENTIFY 0xEC

// Status register bits
#define IDE_SR_BSY 0x80  // Busy
#define IDE_SR_DRDY 0x40 // Drive ready
#define IDE_SR_DF 0x20   // Drive write fault
#define IDE_SR_DSC 0x10  // Drive seek complete
#define IDE_SR_DRQ 0x08  // Data request ready
#define IDE_SR_CORR 0x04 // Corrected data
#define IDE_SR_IDX 0x02  // Index
#define IDE_SR_ERR 0x01  // Error

uint16_t byteswap(uint16_t a)
{
    return ((a & 0x00FF) << 8) | ((a & 0xFF00) >> 8);
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

void IDE_read_sector(uint16_t *buf)
{
    for (uint16_t i = 0; i < 256; i++)
    {
        // IDE_wait_for_data_ready();   // needed at higher speeds?
        buf[i] = byteswap(MEM16(IDE_DATA));
    }
}

void IDE_device_info(uint16_t *buf)
{
    printf("Reading IDE device info...\r\n");
    MEM(IDE_COMMAND) = IDE_CMD_IDENTIFY;
    IDE_wait_for_data_ready();
    IDE_read_sector(buf);

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

int main()
{
    uint16_t buf[256] = {0};

    printf("IDE Demo\r\n");

    printf("reset IDE device\r\n");
    MEM(IDE_COMMAND) = IDE_CMD_RESET;
    IDE_wait_for_device_ready();

    printf("IDE is ready\r\n");

    IDE_device_info(buf);

    printf("read sector\r\n");

    MEM(IDE_SECTOR_START) = 0x01; // sector count starts at 1
    MEM(IDE_SECTOR_COUNT) = 0x01;
    MEM(IDE_COMMAND) = IDE_CMD_READ_SECTOR;
    IDE_wait_for_data_ready();

    IDE_read_sector(buf);

    for (int i = 0; i < 256; i++)
    {
        printf("%d: %04X\r\n", i, buf[i]);
    }

    printf("Done\r\n");

    return 0;
}
