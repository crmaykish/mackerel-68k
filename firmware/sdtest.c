// Mackerel-F SD card bring-up (RAM program, ymodem + run).
//
// Brings the onboard microSD up in SPI mode through the tiny_spi master:
// CMD0/CMD8/ACMD41/CMD58 init, then reads CID/CSD and block 0 and checks the
// 0x55AA boot signature. Proves real SD reads from C -- no FAT, no Linux yet.

#include <stdio.h>
#include "mackerel.h"
#include "spi_tiny.h"

#define SD_CS 0x40 // GPIO bit 6 -> sd_cs (set = card selected, sd_cs = ~gpio[6])

// CS via read-modify-write so the LED bits in the same GPIO register survive.
static void cs_low(void) { MEM(GPIO_BASE) |= SD_CS; }
static void cs_high(void) { MEM(GPIO_BASE) &= ~SD_CS; }

// Clock out a 0xFF and return the received byte (MOSI must idle high for SD).
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

// Read a data block: wait for the 0xFE token, read len bytes, drop the 2 CRC bytes.
static int sd_read_block(uint8_t *buf, int len)
{
    uint8_t t = 0xFF;
    for (int i = 0; i < 2000 && t == 0xFF; i++)
        t = rx();
    if (t != 0xFE)
        return -1;
    for (int i = 0; i < len; i++)
        buf[i] = rx();
    rx();
    rx();
    return 0;
}

static void hexdump(const uint8_t *b, int n)
{
    for (int i = 0; i < n; i++)
        printf("%02X%s", b[i], (i % 16 == 15) ? "\r\n" : " ");
}

int main(void)
{
    uint8_t buf[512], cid[16], csd[16], r7[4], ocr[4];

    printf("\r\n=== %s SD card test (%s %s) ===\r\n", SYSTEM_NAME, __DATE__, __TIME__);

    // Power-up: >=74 clocks with CS high and MOSI high, at <=400 kHz.
    spi_init(128); // ~251 kHz
    cs_high();
    for (int i = 0; i < 10; i++)
        rx();

    // CMD0: reset to idle (enters SPI mode). Needs CRC 0x95.
    uint8_t r1 = 0xFF;
    for (int i = 0; i < 10 && r1 != 0x01; i++)
    {
        cs_low();
        r1 = sd_cmd(0, 0, 0x95);
        cs_high();
        rx();
    }
    printf("CMD0  R1=0x%02X %s\r\n", r1, r1 == 0x01 ? "(idle)" : "(FAIL)");
    if (r1 != 0x01)
        goto fail;

    // CMD8: voltage / pattern check. R7 = R1 + 4 bytes; echo 0x01AA marks a v2 card.
    cs_low();
    r1 = sd_cmd(8, 0x1AA, 0x87);
    for (int i = 0; i < 4; i++)
        r7[i] = rx();
    cs_high();
    rx();
    int v2 = (r1 == 0x01 && r7[2] == 0x01 && r7[3] == 0xAA);
    printf("CMD8  R1=0x%02X echo=%02X%02X %s\r\n", r1, r7[2], r7[3], v2 ? "(v2)" : "(v1/none)");

    // ACMD41: start init, poll until ready (R1=0). HCS bit set for v2 (SDHC support).
    uint32_t hcs = v2 ? 0x40000000UL : 0;
    r1 = 0xFF;
    int tries = 0;
    for (; tries < 1000 && r1 != 0x00; tries++)
    {
        cs_low();
        sd_cmd(55, 0, 0x01);
        r1 = sd_cmd(41, hcs, 0x01);
        cs_high();
        rx();
    }
    printf("ACMD41 R1=0x%02X after %d tries %s\r\n", r1, tries, r1 == 0x00 ? "(ready)" : "(FAIL)");
    if (r1 != 0x00)
        goto fail;

    // CMD58: read OCR, CCS bit (bit30) -> high-capacity (block-addressed) card.
    cs_low();
    r1 = sd_cmd(58, 0, 0x01);
    for (int i = 0; i < 4; i++)
        ocr[i] = rx();
    cs_high();
    rx();
    int sdhc = (ocr[0] & 0x40) != 0;
    printf("CMD58 OCR=%02X%02X%02X%02X %s\r\n", ocr[0], ocr[1], ocr[2], ocr[3],
           sdhc ? "(SDHC/block-addr)" : "(SDSC/byte-addr)");

    // Init done -- speed up the clock for the data reads.
    spi_init(7); // ~4 MHz

    // CMD10: read the 16-byte CID.
    cs_low();
    r1 = sd_cmd(10, 0, 0x01);
    int ok_cid = (r1 == 0x00) && (sd_read_block(cid, 16) == 0);
    cs_high();
    rx();
    if (ok_cid)
    {
        printf("CID: ");
        hexdump(cid, 16);
        printf("  mfr=0x%02X product='%c%c%c%c%c'\r\n", cid[0], cid[3], cid[4], cid[5], cid[6], cid[7]);
    }
    else
        printf("CID read FAIL (R1=0x%02X)\r\n", r1);

    // CMD9: read the 16-byte CSD; for v2 (SDHC) decode capacity.
    cs_low();
    r1 = sd_cmd(9, 0, 0x01);
    int ok_csd = (r1 == 0x00) && (sd_read_block(csd, 16) == 0);
    cs_high();
    rx();
    if (ok_csd)
    {
        printf("CSD: ");
        hexdump(csd, 16);
        if ((csd[0] >> 6) == 1) // CSD version 2.0
        {
            uint32_t c_size = ((uint32_t)(csd[7] & 0x3F) << 16) | (csd[8] << 8) | csd[9];
            printf("  capacity ~ %lu MB\r\n", (unsigned long)((c_size + 1) / 2));
        }
    }
    else
        printf("CSD read FAIL (R1=0x%02X)\r\n", r1);

    // CMD17: read block 0 (LBA 0 = byte 0; same arg either addressing mode).
    cs_low();
    r1 = sd_cmd(17, 0, 0x01);
    int ok_blk = (r1 == 0x00) && (sd_read_block(buf, 512) == 0);
    cs_high();
    rx();
    if (!ok_blk)
    {
        printf("CMD17 block 0 FAIL (R1=0x%02X)\r\n", r1);
        goto fail;
    }
    printf("block 0 [0..31]:\r\n");
    hexdump(buf, 32);
    int sig = (buf[510] == 0x55 && buf[511] == 0xAA);
    printf("sig[510..511]=%02X%02X %s\r\n", buf[510], buf[511], sig ? "(0x55AA OK)" : "(no boot sig)");

    printf("--- SD bring-up: %s ---\r\n", sig ? "PASS" : "read OK, no 0x55AA");
    fflush(stdout);
    MEM(GPIO_BASE) = sig ? 0x00 : 0x3F;
    return sig ? 0 : 1;

fail:
    printf("--- SD bring-up FAILED ---\r\n");
    fflush(stdout);
    MEM(GPIO_BASE) = 0x3F;
    return 1;
}
