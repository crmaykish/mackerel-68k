#include <stdio.h>
#include "sd_spi.h"
#include "spi_tiny.h"

#define SD_CS 0x40 // GPIO bit 6 -> sd_cs (set = selected, sd_cs = ~gpio[6])

static int sdhc = 0; // 1 = block-addressed (SDHC/SDXC), 0 = byte-addressed (SDSC)

static void cs_low(void) { MEM(GPIO_BASE) |= SD_CS; }
static void cs_high(void) { MEM(GPIO_BASE) &= ~SD_CS; }
static uint8_t rx(void) { return spi_transfer(0xFF); }

// Send a command frame, return R1 (the card sends 0xFF until it responds).
static uint8_t sd_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    rx(); // one idle byte before the command (Ncs)
    spi_transfer(0x40 | cmd);
    spi_transfer(arg >> 24);
    spi_transfer(arg >> 16);
    spi_transfer(arg >> 8);
    spi_transfer(arg);
    spi_transfer(crc);

    uint8_t r1 = 0xFF;
    for (int i = 0; i < 10 && (r1 & 0x80); i++)
        r1 = rx();
    return r1;
}

// Wait for the 0xFE data token, read len bytes, drop the 2 CRC bytes.
static int read_data(uint8_t *buf, int len)
{
    uint8_t t = 0xFF;
    for (int i = 0; i < 8192 && t == 0xFF; i++)
        t = rx();
    if (t != 0xFE)
        return -1;
    for (int i = 0; i < len; i++)
        buf[i] = rx();
    rx();
    rx();
    return 0;
}

void sd_spi_set_baud(uint8_t baud) { spi_init(baud); }

bool sd_spi_init(void)
{
    // Power-up: >=74 clocks with CS high and MOSI high, at <=400 kHz.
    spi_init(SD_BAUD_INIT);
    cs_high();
    for (int i = 0; i < 10; i++)
        rx();

    // CMD0: reset to idle (enters SPI mode).
    uint8_t r1 = 0xFF;
    for (int i = 0; i < 10 && r1 != 0x01; i++)
    {
        cs_low();
        r1 = sd_cmd(0, 0, 0x95);
        cs_high();
        rx();
    }
    if (r1 != 0x01)
        return false;

    // CMD8: voltage/pattern check; echo 0x01AA marks a v2 card.
    cs_low();
    r1 = sd_cmd(8, 0x1AA, 0x87);
    uint8_t r7[4];
    for (int i = 0; i < 4; i++)
        r7[i] = rx();
    cs_high();
    rx();
    int v2 = (r1 == 0x01 && r7[2] == 0x01 && r7[3] == 0xAA);

    // ACMD41: init, poll until ready. HCS bit set for v2 (SDHC support).
    uint32_t hcs = v2 ? 0x40000000UL : 0;
    r1 = 0xFF;
    for (int i = 0; i < 1000 && r1 != 0x00; i++)
    {
        cs_low();
        sd_cmd(55, 0, 0x01);
        r1 = sd_cmd(41, hcs, 0x01);
        cs_high();
        rx();
    }
    if (r1 != 0x00)
        return false;

    // CMD58: CCS bit (bit30) -> high-capacity (block-addressed) card.
    cs_low();
    sd_cmd(58, 0, 0x01);
    uint8_t ocr[4];
    for (int i = 0; i < 4; i++)
        ocr[i] = rx();
    cs_high();
    rx();
    sdhc = (ocr[0] & 0x40) != 0;

    spi_init(SD_BAUD_DATA);
    return true;
}

int sd_spi_read(uint32_t lba, uint8_t *buf, uint32_t count)
{
    for (uint32_t b = 0; b < count; b++)
    {
        uint32_t addr = sdhc ? (lba + b) : ((lba + b) * 512);
        cs_low();
        uint8_t r1 = sd_cmd(17, addr, 0x01);
        int ok = (r1 == 0x00) && (read_data(buf + b * 512, 512) == 0);
        cs_high();
        rx();
        if (!ok)
            return -1;
    }
    return 0;
}

void sd_spi_print_info(void)
{
    uint8_t cid[16], csd[16];

    cs_low();
    uint8_t r1 = sd_cmd(10, 0, 0x01);
    int ok = (r1 == 0x00) && (read_data(cid, 16) == 0);
    cs_high();
    rx();
    if (ok)
        printf("SD: mfr=0x%02X product='%c%c%c%c%c'\r\n", cid[0], cid[3], cid[4], cid[5], cid[6], cid[7]);

    cs_low();
    r1 = sd_cmd(9, 0, 0x01);
    ok = (r1 == 0x00) && (read_data(csd, 16) == 0);
    cs_high();
    rx();
    if (ok && (csd[0] >> 6) == 1)
    {
        uint32_t c_size = ((uint32_t)(csd[7] & 0x3F) << 16) | (csd[8] << 8) | csd[9];
        printf("SD: capacity ~ %lu MB (%s)\r\n", (unsigned long)((c_size + 1) / 2), sdhc ? "SDHC" : "SDSC");
    }
}
