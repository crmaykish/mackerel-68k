// W5500 Ethernet Controller
// Handles the whole TCP/IP stack in hardware, bootloader just sends it SPI commands

#ifndef _W5500_H
#define _W5500_H

#include <stdbool.h>
#include "mackerel.h"

// Set the W5500 with a static IP
// Return false if W5500 is not detected
bool w5500_init(const uint8_t mac[6], const uint8_t ip[4],
                const uint8_t gw[4], const uint8_t sub[4]);

// Open a socket connection to dip:dport
// Return false on failure/timeut
bool w5500_tcp_connect(const uint8_t dip[4], uint16_t dport, uint16_t sport);

// Receive len bytes into dst
// Return false if peer closes early
bool w5500_tcp_recv(uint8_t *dst, uint32_t len);

// Disconnect and close the socket
void w5500_tcp_close(void);

#endif
