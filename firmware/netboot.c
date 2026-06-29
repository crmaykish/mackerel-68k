#include <stdio.h>
#include "netboot.h"
#include "w5500.h"
#include "mackerel.h"

// ===== Network configuration =====
// Set the IP of Mackerel and the netboot server (your PC)
#define NETBOOT_BOARD_IP    192, 168, 1, 199
#define NETBOOT_SERVER_IP   192, 168, 1, 200
#define NETBOOT_GATEWAY     192, 168, 1, 1
#define NETBOOT_NETMASK     255, 255, 255, 0
#define NETBOOT_SERVER_PORT 5000
// 02=locally-administered unicast; 4D4B5246="MKRF"; 01=unit#
#define NETBOOT_MAC         0x02, 0x4d, 0x4b, 0x52, 0x46, 0x01  
// =================================================================

// W5500 register/connect calls take octet arrays, so expand the macros into them.
static const uint8_t mac[6] = {NETBOOT_MAC};
static const uint8_t ip[4] = {NETBOOT_BOARD_IP};
static const uint8_t gw[4] = {NETBOOT_GATEWAY};
static const uint8_t sub[4] = {NETBOOT_NETMASK};
static const uint8_t srv[4] = {NETBOOT_SERVER_IP};

#define NETBOOT_SRC_PORT 3333 // local TCP source port (arbitrary)

// Pull one length-prefixed blob ([u32 big-endian length][bytes]) from the open TCP
// socket into RAM at addr. Returns false on a short/failed read.
static bool netboot_blob(const char *name, uint32_t addr)
{
    uint8_t lb[4];
    if (!w5500_tcp_recv(lb, 4))
    {
        printf("ERROR: %s length recv failed\r\n", name);
        return false;
    }
    uint32_t len = ((uint32_t)lb[0] << 24) | ((uint32_t)lb[1] << 16) | ((uint32_t)lb[2] << 8) | lb[3];
    printf("%s: %lu bytes -> 0x%lX\r\n", name, (unsigned long)len, (unsigned long)addr);
    if (!w5500_tcp_recv((uint8_t *)addr, len))
    {
        printf("ERROR: %s data recv failed (short read)\r\n", name);
        return false;
    }
    return true;
}

// Fill RAM with the kernel image + ROMfs over the network.
bool netboot_load(void)
{
    printf("Netboot: initializing W5500...\r\n");
    if (!w5500_init(mac, ip, gw, sub))
    {
        printf("ERROR: W5500 not detected (VERSIONR != 0x04)\r\n");
        return false;
    }

    printf("Connecting to %d.%d.%d.%d:%d ...\r\n", srv[0], srv[1], srv[2], srv[3], NETBOOT_SERVER_PORT);
    if (!w5500_tcp_connect(srv, NETBOOT_SERVER_PORT, NETBOOT_SRC_PORT))
    {
        printf("ERROR: TCP connect failed (server running? cable in?)\r\n");
        return false;
    }

    bool image_ok = netboot_blob("IMAGE.BIN", PROGRAM_START);
    #ifdef MACKEREL_F
    bool romfs_ok = netboot_blob("ROMFS.BIN", ROMFS_LOAD_ADDR);
    bool ok = image_ok && romfs_ok;
    #else
    bool ok = image_ok;
    #endif

    w5500_tcp_close();
    if (ok)
        printf("Netboot load complete.\r\n");
    return ok;
}