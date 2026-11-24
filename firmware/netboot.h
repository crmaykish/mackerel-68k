// Mackerel-F network boot
#ifndef _NETBOOT_H
#define _NETBOOT_H

#include <stdint.h>

// Fetch IMAGE.BIN into RAM at PROGRAM_START over raw TCP from the configured
// netboot server. Returns the number of bytes loaded, 0 on failure.
uint32_t netboot_load(void);

#endif
