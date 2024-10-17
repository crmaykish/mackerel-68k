#include "ide.h"
#include "mackerel.h"
#include <stdio.h>

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
        buf[i] = MEM16(IDE_DATA);
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