// Mackerel-10 W5500 network RX test (RAM program, loaded over ymodem then `run`).
//
// Validates the full W5500 TCP path through the new CPLD SPI master before the
// bootloader gains `netboot`: bring the NIC up with a static IP, connect to the
// netboot server on the PC, receive a length-prefixed ramp blob, and verify every
// byte (byte[i] == i & 0xFF). Start the server first, e.g.:
//   tools/netboot_server.py /tmp/ramp64k.bin --single -p 5000

#include <stdio.h>
#include "mackerel.h"
#include "w5500.h"

#define TEST_MAX (64 * 1024)
static uint8_t rxbuf[TEST_MAX];

// Matches netboot.c / netboot_server.py defaults (this PC = 192.168.1.200).
static const uint8_t mac[6] = {0x02, 0x4d, 0x4b, 0x52, 0x46, 0x01};
static const uint8_t ip[4] = {192, 168, 1, 199};
static const uint8_t gw[4] = {192, 168, 1, 1};
static const uint8_t sub[4] = {255, 255, 255, 0};
static const uint8_t srv[4] = {192, 168, 1, 200};
#define SRV_PORT 5000
#define SRC_PORT 3333

int main(void)
{
    printf("\r\n=== %s W5500 network test (%s %s) ===\r\n", SYSTEM_NAME, __DATE__, __TIME__);

    if (!w5500_init(mac, ip, gw, sub)) {
        printf("*** FAIL: w5500_init (VERSIONR != 0x04) ***\r\n");
        return 1;
    }
    printf("W5500 up @ 192.168.1.199; connecting to 192.168.1.200:%d ...\r\n", SRV_PORT);

    if (!w5500_tcp_connect(srv, SRV_PORT, SRC_PORT)) {
        printf("*** FAIL: tcp_connect (server running? link up?) ***\r\n");
        return 1;
    }
    printf("TCP established. Receiving blob...\r\n");

    uint8_t lb[4];
    if (!w5500_tcp_recv(lb, 4)) {
        printf("*** FAIL: length recv ***\r\n");
        w5500_tcp_close();
        return 1;
    }
    uint32_t len = ((uint32_t)lb[0] << 24) | ((uint32_t)lb[1] << 16) | ((uint32_t)lb[2] << 8) | lb[3];
    printf("blob = %lu bytes\r\n", (unsigned long)len);

    if (len == 0 || len > TEST_MAX) {
        printf("*** FAIL: bad length %lu (max %d) ***\r\n", (unsigned long)len, TEST_MAX);
        w5500_tcp_close();
        return 1;
    }

    if (!w5500_tcp_recv(rxbuf, len)) {
        printf("*** FAIL: data recv (short read) ***\r\n");
        w5500_tcp_close();
        return 1;
    }
    w5500_tcp_close();

    printf("rx[0..31]:");
    for (int i = 0; i < 32; i++) printf(" %02X", rxbuf[i]);
    printf("\r\n");

    uint32_t bad = 0, first = 0;
    for (uint32_t i = 0; i < len; i++) {
        if (rxbuf[i] != (uint8_t)(i & 0xFF)) {
            if (!bad) first = i;
            bad++;
        }
    }

    if (bad == 0) {
        printf("*** PASS: received + verified %lu bytes over Ethernet ***\r\n", (unsigned long)len);
    } else {
        printf("*** FAIL: %lu/%lu wrong, first @ %lu (got 0x%02X want 0x%02X) ***\r\n",
               (unsigned long)bad, (unsigned long)len, (unsigned long)first,
               rxbuf[first], (uint8_t)(first & 0xFF));
    }
    fflush(stdout);
    return bad ? 1 : 0;
}
