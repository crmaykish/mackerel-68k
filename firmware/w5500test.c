// Mackerel-10 W5500 SPI smoke test (RAM program, loaded over ymodem then `run`).
//
// Reads the W5500's VERSIONR register (0x0039 in the common register block) over
// the CPLD tiny_spi master. A healthy W5500 always returns 0x04 -- the simplest
// unambiguous proof that the SPI master works end-to-end and the NIC is alive.

#include <stdio.h>
#include "mackerel.h"
#include "spi_tiny.h"

#define W5500_VERSIONR 0x0039

// CS register: write 1 -> nic_cs_n low (W5500 selected), 0 -> deselected.
static void nic_select(int on) { MEM(SPI_NIC_CS) = on ? 1 : 0; }

// W5500 common-register read frame: [addr_hi][addr_lo][control][data...].
// control = BSB(common=00000) | RWB(read=0) | OM(VDM=00) = 0x00.
// Dummy read byte is 0xFF (W5500 reads bit-slip on 0 bits during the read phase).
static uint8_t w5500_read(uint16_t addr)
{
    nic_select(1);
    spi_transfer((addr >> 8) & 0xFF);
    spi_transfer(addr & 0xFF);
    spi_transfer(0x00);
    uint8_t v = spi_transfer(0xFF);
    nic_select(0);
    return v;
}

int main(void)
{
    printf("\r\n=== %s W5500 SPI test (%s %s) ===\r\n", SYSTEM_NAME, __DATE__, __TIME__);

    nic_select(0);   // start deselected
    spi_init(9);     // SCK = 20 MHz / (2*(9+1)) = 1 MHz

    uint8_t st = MEM(SPI_STATUS);
    printf("SPI STATUS = 0x%02X (expect 0x03 idle)\r\n", st);

    uint8_t ver = w5500_read(W5500_VERSIONR);
    printf("W5500 VERSIONR = 0x%02X (expect 0x04)\r\n", ver);

    if (ver == 0x04) {
        printf("*** PASS: W5500 alive over the CPLD SPI master ***\r\n");
    } else if (ver == 0x00 || ver == 0xFF) {
        printf("*** FAIL: 0x%02X = no MISO data (check MISO/CS/power/ground) ***\r\n", ver);
    } else {
        printf("*** FAIL: 0x%02X = SPI link issue (mode/clock/wiring) ***\r\n", ver);
    }

    fflush(stdout);
    return (ver == 0x04) ? 0 : 1;
}
