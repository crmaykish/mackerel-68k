#include "mackerel.h"
#include "ide.h"
#include <stdio.h>
#include <string.h>

void __attribute__((interrupt)) timer_isr()
{
    // duart_putc('.'); 
}

void __attribute__((interrupt)) ide_isr()
{
    uint8_t status = MEM(IDE_STATUS);
    duart_putc('_'); 
}

int main()
{
    uint16_t buf[256] = {0};

    set_exception_handler(25, timer_isr);
    set_exception_handler(28, ide_isr);

    set_interrupts(true);

    printf("IDE Demo\r\n");

    printf("reset IDE device\r\n");
    MEM(IDE_COMMAND) = IDE_CMD_RESET;
    IDE_wait_for_device_ready();

    printf("IDE is ready\r\n");

    IDE_device_info(buf);

    printf("read sector 1\r\n");
    IDE_read_sector(buf, 1);

    for (int i = 0; i < 256; i++)
    {
        printf("%d: %04X\r\n", i, buf[i]);
    }

    printf("read sector 2\r\n");
    IDE_read_sector(buf, 2);

    for (int i = 0; i < 256; i++)
    {
        printf("%d: %04X\r\n", i, buf[i]);
    }

    printf("Done\r\n");

    return 0;
}
