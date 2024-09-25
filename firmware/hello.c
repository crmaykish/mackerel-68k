#include "mackerel.h"
#include "ide.h"
#include <stdio.h>
#include <string.h>

int main()
{
    uint16_t buf[256] = {0};

    printf("IDE Demo\r\n");

    printf("reset IDE device\r\n");
    MEM(IDE_COMMAND) = IDE_CMD_RESET;
    IDE_wait_for_device_ready();

    printf("IDE is ready\r\n");

    IDE_device_info(buf);

    printf("read sector 1\r\n");

    MEM(IDE_SECTOR_START) = 0x01; // IDE sector count starts at 1
    MEM(IDE_SECTOR_COUNT) = 1;
    MEM(IDE_COMMAND) = IDE_CMD_READ_SECTOR;
    IDE_wait_for_data_ready();

    IDE_read_sector(buf);

    for (int i = 0; i < 256; i++)
    {
        printf("%d: %04X\r\n", i, buf[i]);
    }

    printf("write sector 2\r\n");

    MEM(IDE_SECTOR_START) = 0x02;
    MEM(IDE_SECTOR_COUNT) = 1;
    MEM(IDE_COMMAND) = IDE_CMD_WRITE_SECTOR;
    IDE_wait_for_data_ready();

    for (int i = 0; i < 256; i++)
    {
        IDE_wait_for_data_ready();
        MEM16(IDE_DATA) = byteswap(0xABCD);
        // printf("%d: %04X\r\n", i, buf[i]);
    }

    // Wait while IDE is busy
    while (MEM(IDE_STATUS) & IDE_SR_BSY)
    {
        // duart_putc('.');
    }

    printf("read sector 2\r\n");

    MEM(IDE_SECTOR_START) = 0x02; // sector count starts at 1
    MEM(IDE_SECTOR_COUNT) = 1;
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
