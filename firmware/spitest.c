// Mackerel-F SPI smoke test (RAM program, loaded over ymodem then `run`).
//
// Proves the tiny_spi driver from C: status reads, the polled transfer engine
// runs a byte to completion, and the SD chip-select (GPIO bit 6) toggles. The
// SD card stays deselected, so MISO idles high (board pull-up) -> reads 0xFF.
// SD protocol comes next in sdtest.

#include <stdio.h>
#include "mackerel.h"
#include "spi_tiny.h"

#define SD_CS 0x40 // GPIO bit 6 -> sd_cs (set = card selected, sd_cs = ~gpio[6])

static int checks = 0, fails = 0;

static void check(const char *name, int ok)
{
    checks++;
    if (!ok)
        fails++;
    printf("  [%s] %s\r\n", ok ? "PASS" : "FAIL", name);
}

// CS via read-modify-write so the LED bits in the same GPIO register survive.
static void sd_cs_low(void) { MEM(GPIO_BASE) |= SD_CS; }   // select
static void sd_cs_high(void) { MEM(GPIO_BASE) &= ~SD_CS; } // deselect

int main(void)
{
    printf("\r\n=== %s SPI smoke test (%s %s) ===\r\n", SYSTEM_NAME, __DATE__, __TIME__);

    sd_cs_high();
    spi_init(128); // ~251 kHz

    uint8_t st = MEM(SPI_STATUS);
    printf("STATUS = 0x%02X\r\n", st);
    check("status idle (TXR|TXE = 0x03)", st == 0x03);

    uint8_t r1 = spi_transfer(0xFF);
    uint8_t r2 = spi_transfer(0x00);
    uint8_t r3 = spi_transfer(0x55);
    printf("xfer FF/00/55 -> %02X %02X %02X\r\n", r1, r2, r3);
    check("transfers return 0xFF (deselected, MISO high)", r1 == 0xFF && r2 == 0xFF && r3 == 0xFF);
    check("status idle after xfers", MEM(SPI_STATUS) == 0x03);

    sd_cs_low();
    uint8_t g_lo = MEM(GPIO_BASE);
    sd_cs_high();
    uint8_t g_hi = MEM(GPIO_BASE);
    printf("GPIO CS sel/desel -> %02X / %02X\r\n", g_lo, g_hi);
    check("CS bit sets", (g_lo & SD_CS) != 0);
    check("CS bit clears", (g_hi & SD_CS) == 0);

    printf("--- %d checks, %d FAILED: %s ---\r\n", checks, fails, fails ? "*** FAIL ***" : "ALL PASS");
    fflush(stdout);

    MEM(GPIO_BASE) = fails ? 0x3F : 0x00; // LEDs: all on = fail, off = pass
    return fails;
}
