// Mackerel-F network boot
#ifndef _NETBOOT_H
#define _NETBOOT_H

#include <stdbool.h>

// Fetch IMAGE.BIN into RAM over raw TCP from the configured netboot server
bool netboot_load(void);

#endif
