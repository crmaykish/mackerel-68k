#include "w5500.h"

// Everything below is board-independent. This block is the ONLY per-board part: the
// W5500 SPI master's register addresses, the baud value, and which register/bit drives
// the chip-select. Setting NIC_CS_MASK in NIC_CS_REG asserts CS (active low) on both.
#ifdef MACKEREL_10
// W5500 on the system-controller tiny_spi (LDS byte lane; odd-address regs from
// spi_tiny.h). CS is a dedicated 1-bit register. SCLK = 20 MHz / (2*(baud+1)).
#include "spi_tiny.h"
#define NIC_TXDATA SPI_TXDATA
#define NIC_RXDATA SPI_RXDATA
#define NIC_STATUS SPI_STATUS
#define NIC_BAUD SPI_BAUD
#define NIC_TXE SPI_TXE
#define NIC_CS_REG SPI_NIC_CS
#define NIC_CS_MASK 0x01
#define W5500_SPI_BAUD 0    // 10 MHz (CLK/2)
#else
// Mackerel-F: W5500 on a 2nd tiny_spi (x4 stride). CS is bit 7 of the GPIO register
// (shared with the LEDs + SD chip-select), so it must be read-modify-written.
#define NIC_RXDATA (SPI2_BASE + 0)
#define NIC_TXDATA (SPI2_BASE + 4)
#define NIC_STATUS (SPI2_BASE + 8)
#define NIC_BAUD (SPI2_BASE + 16)
#define NIC_TXE 0x01        // engine idle (transfer complete)
#define NIC_CS_REG GPIO_BASE
#define NIC_CS_MASK 0x80    // gpio[7] -> cs_spi_nic = ~gpio[7]
#define W5500_SPI_BAUD 1    // SCLK = clk_soc/(2*(baud+1)); 1 = ~18.9 MHz, 0 = ~37.8 MHz
#endif

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

// SPI transport -- fully board-independent (the defines above are the only per-board
// part). One full-duplex byte over the master, then the chip-select helpers (set the
// bit to select; the RMW preserves Mackerel-F's shared GPIO byte).
static uint8_t spi2(uint8_t tx)
{
    MEM(NIC_TXDATA) = tx;
    while (!(MEM(NIC_STATUS) & NIC_TXE)) {}
    return MEM(NIC_RXDATA);
}

static void cs_low(void) { MEM(NIC_CS_REG) |= NIC_CS_MASK; }   // select
static void cs_high(void) { MEM(NIC_CS_REG) &= ~NIC_CS_MASK; } // deselect

static void w5500_spi_init(void) { MEM(NIC_BAUD) = W5500_SPI_BAUD; }

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
    w5500_spi_init();
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
