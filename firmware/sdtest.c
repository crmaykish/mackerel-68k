// Mackerel-F SD card smoke test (RAM program, ymodem + run).
//
// Exercises the sd_spi driver: init, card identity, read block 0, verify the
// 0x55AA boot signature. SD protocol lives in sd_spi.c.

#include <stdio.h>
#include "mackerel.h"
#include "sd_spi.h"

int main(void)
{
    uint8_t buf[512];

    printf("\r\n=== %s SD card test (%s %s) ===\r\n", SYSTEM_NAME, __DATE__, __TIME__);

    if (!sd_spi_init())
    {
        printf("SD init FAILED\r\n");
        MEM(GPIO_BASE) = 0x3F;
        return 1;
    }
    printf("SD init OK\r\n");
    sd_spi_print_info();

    if (sd_spi_read(0, buf, 1) != 0)
    {
        printf("block 0 read FAILED\r\n");
        MEM(GPIO_BASE) = 0x3F;
        return 1;
    }

    int sig = (buf[510] == 0x55 && buf[511] == 0xAA);
    printf("block 0 [0..31]:\r\n");
    for (int i = 0; i < 32; i++)
        printf("%02X%s", buf[i], (i % 16 == 15) ? "\r\n" : " ");
    printf("sig[510..511]=%02X%02X %s\r\n", buf[510], buf[511], sig ? "(0x55AA OK)" : "(no sig)");

    printf("--- SD smoke test: %s ---\r\n", sig ? "PASS" : "read OK, no 0x55AA");
    fflush(stdout);
    MEM(GPIO_BASE) = sig ? 0x00 : 0x3F;
    return sig ? 0 : 1;
}
