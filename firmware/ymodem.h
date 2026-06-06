#ifndef _YMODEM_H
#define _YMODEM_H

#include <stdint.h>

// Receive a file over the serial port using YMODEM protocol.
// Bytes are written to `buf` up to `bufsz` in length.
//
// Returns the number of bytes read on success, else error code
long ymodem_recv(uint8_t *buf, uint32_t bufsz, char *name, uint32_t *size);

#define YM_ERR_TIMEOUT (-1)
#define YM_ERR_CANCEL  (-2)

#endif
