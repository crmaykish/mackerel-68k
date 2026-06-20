#include "w5500.h"

// W5500 is on its own tiny_spi master
#define SPI2_RXDATA (SPI2_BASE + 0)
#define SPI2_TXDATA (SPI2_BASE + 4)
#define SPI2_STATUS (SPI2_BASE + 8)
#define SPI2_BAUD (SPI2_BASE + 16)
#define SPI2_TXE 0x01 // engine idle (transfer complete)
#define SPI2_TXR 0x02 // ready to accept the next byte (holding register empty)

#define NIC_CS 0x80 // GPIO bit 7 -> cs_spi_nic = ~gpio[7] (set = selected)

#define W5500_SPI_BAUD 1 // SCLK = clk_soc/(2*(baud+1)); 1 = ~16.2 MHz, 0 = ~32.4 MHz

// W5500 SPI frame: [addr_hi][addr_lo][control][data...], control = BSB<<3 | RW | OM.
#define BSB_COMMON 0x00
#define BSB_S0_REG 0x01
#define BSB_S0_RX 0x03
#define RW_WRITE 0x04 // 0 = read, in VDM (variable length) mode

// Common registers
#define W5_MR 0x0000
#define W5_GAR 0x0001
#define W5_SUBR 0x0005
#define W5_SHAR 0x0009
#define W5_SIPR 0x000F
#define W5_VERSIONR 0x0039

// Socket 0 registers
#define S0_MR 0x0000
#define S0_CR 0x0001
#define S0_SR 0x0003
#define S0_PORT 0x0004
#define S0_DIPR 0x000C
#define S0_DPORT 0x0010
#define S0_RX_RSR 0x0026
#define S0_RX_RD 0x0028

// Sn_CR commands / Sn_SR status / Sn_MR mode
#define CR_OPEN 0x01
#define CR_CONNECT 0x04
#define CR_DISCON 0x08
#define CR_CLOSE 0x10
#define CR_RECV 0x40
#define SR_CLOSED 0x00
#define SR_INIT 0x13
#define SR_ESTABLISHED 0x17
#define SR_CLOSE_WAIT 0x1C
#define MR_TCP 0x01

static uint8_t spi2(uint8_t tx)
{
    MEM(SPI2_TXDATA) = tx;
    while (!(MEM(SPI2_STATUS) & SPI2_TXE)) {}
    return MEM(SPI2_RXDATA);
}

static void cs_low(void) { MEM(GPIO_BASE) |= NIC_CS; }
static void cs_high(void) { MEM(GPIO_BASE) &= ~NIC_CS; }

static void w5_write(uint16_t addr, uint8_t bsb, const uint8_t *data, int len)
{
    cs_low();
    spi2(addr >> 8);
    spi2(addr & 0xFF);
    spi2((bsb << 3) | RW_WRITE);
    for (int i = 0; i < len; i++)
        spi2(data[i]);
    cs_high();
}

static void w5_read(uint16_t addr, uint8_t bsb, uint8_t *data, int len)
{
    cs_low();
    spi2(addr >> 8);
    spi2(addr & 0xFF);
    spi2(bsb << 3); // RW = 0 (read)
    // Dummy bytes MUST be 0xFF: a 0-bit on MOSI during a W5500 read slips bits.
    for (int i = 0; i < len; i++)
        data[i] = spi2(0xFF);
    cs_high();
}

static uint8_t w5_rb(uint16_t a, uint8_t b)
{
    uint8_t v;
    w5_read(a, b, &v, 1);
    return v;
}
static void w5_wb(uint16_t a, uint8_t b, uint8_t v) { w5_write(a, b, &v, 1); }
static uint16_t w5_rw(uint16_t a, uint8_t b)
{
    uint8_t t[2];
    w5_read(a, b, t, 2);
    return (t[0] << 8) | t[1];
}
static void w5_ww(uint16_t a, uint8_t b, uint16_t v)
{
    uint8_t t[2] = {v >> 8, v & 0xFF};
    w5_write(a, b, t, 2);
}

bool w5500_init(const uint8_t mac[6], const uint8_t ip[4], const uint8_t gw[4], const uint8_t sub[4])
{
    MEM(SPI2_BAUD) = W5500_SPI_BAUD;
    cs_high();

    w5_wb(W5_MR, BSB_COMMON, 0x80); // software reset
    for (int i = 0; i < 100000 && (w5_rb(W5_MR, BSB_COMMON) & 0x80); i++) {}

    if (w5_rb(W5_VERSIONR, BSB_COMMON) != 0x04)
        return false;

    w5_write(W5_SHAR, BSB_COMMON, mac, 6);
    w5_write(W5_SIPR, BSB_COMMON, ip, 4);
    w5_write(W5_GAR, BSB_COMMON, gw, 4);
    w5_write(W5_SUBR, BSB_COMMON, sub, 4);
    return true;
}

bool w5500_tcp_connect(const uint8_t dip[4], uint16_t dport, uint16_t sport)
{
    w5_wb(S0_MR, BSB_S0_REG, MR_TCP);
    w5_ww(S0_PORT, BSB_S0_REG, sport);
    w5_wb(S0_CR, BSB_S0_REG, CR_OPEN);
    for (int i = 0; i < 100000 && w5_rb(S0_SR, BSB_S0_REG) != SR_INIT; i++) {}
    if (w5_rb(S0_SR, BSB_S0_REG) != SR_INIT)
        return false;

    w5_write(S0_DIPR, BSB_S0_REG, dip, 4);
    w5_ww(S0_DPORT, BSB_S0_REG, dport);
    w5_wb(S0_CR, BSB_S0_REG, CR_CONNECT);

    for (uint32_t i = 0; i < 30000000; i++)
    {
        uint8_t sr = w5_rb(S0_SR, BSB_S0_REG);
        if (sr == SR_ESTABLISHED)
            return true;
        if (sr == SR_CLOSED) // connect refused / timed out
            return false;
    }
    return false;
}

bool w5500_tcp_recv(uint8_t *dst, uint32_t len)
{
    uint32_t got = 0;
    while (got < len)
    {
        uint16_t rsr = w5_rw(S0_RX_RSR, BSB_S0_REG);
        if (rsr == 0)
        {
            // Nothing buffered. If the peer has closed and drained, it's a short read.
            uint8_t sr = w5_rb(S0_SR, BSB_S0_REG);
            if ((sr == SR_CLOSE_WAIT || sr == SR_CLOSED) && w5_rw(S0_RX_RSR, BSB_S0_REG) == 0)
                return false;
            continue;
        }
        uint32_t want = len - got;
        uint16_t chunk = (rsr < want) ? rsr : (uint16_t)want;

        // W5500 auto-wraps within the socket RX buffer, so a single burst at the
        // raw read pointer is fine -- no manual masking/splitting needed.
        uint16_t rd = w5_rw(S0_RX_RD, BSB_S0_REG);
        w5_read(rd, BSB_S0_RX, dst + got, chunk);
        w5_ww(S0_RX_RD, BSB_S0_REG, rd + chunk);
        w5_wb(S0_CR, BSB_S0_REG, CR_RECV);

        got += chunk;
    }
    return true;
}

void w5500_tcp_close(void)
{
    w5_wb(S0_CR, BSB_S0_REG, CR_DISCON);
    w5_wb(S0_CR, BSB_S0_REG, CR_CLOSE);
}
