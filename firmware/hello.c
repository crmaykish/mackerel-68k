#include "mackerel.h"
#include <stdio.h>

#define IDE_BASE 0x100000
#define IDE_DATA IDE_BASE + 0x00
#define IDE_STATUS IDE_BASE + 0x0F
#define IDE_COMMAND IDE_BASE + 0x0F

#define IDE_CMD_IDENTIFY 0xEC

#define ATA_SR_BSY 0x80  // Busy
#define ATA_SR_DRDY 0x40 // Drive ready
#define ATA_SR_DF 0x20   // Drive write fault
#define ATA_SR_DSC 0x10  // Drive seek complete
#define ATA_SR_DRQ 0x08  // Data request ready
#define ATA_SR_CORR 0x04 // Corrected data
#define ATA_SR_IDX 0x02  // Index
#define ATA_SR_ERR 0x01  // Error

uint16_t byteswap(uint16_t a)
{
    return ((a & 0x00FF) << 8) | ((a & 0xFF00) >> 8);
}

uint8_t ide_status()
{
    uint8_t status = MEM(IDE_STATUS);

    printf("BSY: %d\r\n", (status & ATA_SR_BSY) ? 1 : 0);
    printf("DRDY: %d\r\n", (status & ATA_SR_DRDY) ? 1 : 0);
    printf("DF: %d\r\n", status & ATA_SR_DF);
    printf("DSC: %d\r\n", (status & ATA_SR_DSC) ? 1 : 0);
    printf("DRQ: %d\r\n", (status & ATA_SR_DRQ) ? 1 : 0);
    printf("CORR: %d\r\n", (status & ATA_SR_CORR) ? 1 : 0);
    printf("IDX: %d\r\n", (status & ATA_SR_IDX) ? 1 : 0);
    printf("ERR: %d\r\n", (status & ATA_SR_ERR) ? 1 : 0);

    return status;
}

uint16_t buf[256] = {0};

int main()
{
    printf("IDE test\r\n");

    while ((MEM(IDE_STATUS) & ATA_SR_DRDY) == 0)
    {
    }

    printf("IDE is ready\r\n");

    // ide_status();

    printf("send ID command\r\n");
    MEM(IDE_COMMAND) = IDE_CMD_IDENTIFY;

    while ((MEM(IDE_STATUS) & ATA_SR_DRQ) == 0)
    {
    }

    printf("device has data\r\n");

    // ide_status();

    for (uint16_t i = 0; i < 256; i++)
    {
        // TODO: wait for a ready signal?
        buf[i] = 
        printf("%d: %X\r\n", i, byteswap(MEM16(IDE_DATA)));
    }

    return 0;
}
