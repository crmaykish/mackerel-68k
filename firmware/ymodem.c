#include "ymodem.h"
#include "mackerel.h"
#include "uart.h"

#define SOH 0x01 // 128-byte block
#define STX 0x02 // 1024-byte block
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
#define CRC_C 'C'

// YMODEM polls for x milliseconds. Calculate poll count based on CPU clock
#define YM_POLL_CYCLES 20UL
#define YM_POLLS_PER_MS ((CPU_CLK_HZ / 1000UL) / YM_POLL_CYCLES)

#define T_BLOCK_MS 1000 // wait for a block to start / between blocks
#define T_BYTE_MS 500   // wait for the next byte within a block (sender stall)
#define T_FLUSH_MS 50   // drain leftovers after a rejected block
#define MAX_IDLE 10     // give up after this many block-start timeouts (~10s)

// RX one byte, poll for up to ms milliseconds
static int rx_ms(uint32_t ms)
{
    uint32_t polls = ms * YM_POLLS_PER_MS;
    for (uint32_t i = 0; i < polls; i++)
    {
        if (uart_rx_ready())
            return (uint8_t)uart_getc();
    }
    return -1;
}

// Calculate CRC-16 XMODEM checksum
static uint16_t crc16(const uint8_t *p, int n)
{
    uint16_t crc = 0;
    for (int i = 0; i < n; i++)
    {
        crc ^= (uint16_t)p[i] << 8;
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    return crc;
}

long ymodem_recv(uint8_t *buf, uint32_t bufsz, char *name, uint32_t *size)
{
    static uint8_t blk[1024];
    uint32_t total = 0;
    uint32_t fsize = 0;
    uint8_t expect = 0;   // next block number we expect (0 = header)
    bool have_header = false;
    bool trailer = false; // after EOT: awaiting final null header block
    int idle = 0;
    long result;

    name[0] = 0;
    *size = 0;

    // Make sure are off during the transfer
    set_interrupts(false);

    uart_putc(CRC_C); // kick off: request CRC mode

    for (;;)
    {
        int hdr = rx_ms(T_BLOCK_MS);
        if (hdr < 0)
        {
            if (++idle > MAX_IDLE)
            {
                result = YM_ERR_TIMEOUT;
                break;
            }
            // Re-prompt: 'C' while negotiating CRC, NAK once in the data flow.
            uart_putc((!have_header || trailer) ? CRC_C : NAK);
            continue;
        }
        idle = 0;

        if (hdr == EOT)
        {
            uart_putc(ACK);
            trailer = true;
            expect = 0;
            uart_putc(CRC_C); // request the trailing null header block
            continue;
        }
        if (hdr == CAN)
        {
            if (rx_ms(T_BLOCK_MS) == CAN)
            {
                result = YM_ERR_CANCEL;
                break;
            }
            continue;
        }
        if (hdr != SOH && hdr != STX)
            continue; // junk between blocks

        int len = (hdr == SOH) ? 128 : 1024;
        int bn = rx_ms(T_BYTE_MS);
        int bnc = rx_ms(T_BYTE_MS);
        bool ok = (bn >= 0 && bnc >= 0 && ((bn + bnc) & 0xFF) == 0xFF);
        for (int i = 0; i < len; i++)
        {
            int d = rx_ms(T_BYTE_MS);
            if (d < 0)
            {
                ok = false;
                break;
            }
            blk[i] = (uint8_t)d;
        }
        int ch = rx_ms(T_BYTE_MS);
        int cl = rx_ms(T_BYTE_MS);
        if (ch < 0 || cl < 0)
            ok = false;
        if (ok && crc16(blk, len) != (uint16_t)((ch << 8) | cl))
            ok = false;

        if (!ok)
        {
            while (rx_ms(T_FLUSH_MS) >= 0) // drain the rest of the bad block
                ;
            uart_putc(NAK);
            continue;
        }

        if ((uint8_t)bn == expect)
        {
            if (trailer)
            {
                uart_putc(ACK); // final null header -> batch complete
                *size = fsize;
                result = (long)total;
                break;
            }
            if (!have_header) // block 0: "filename\0size ..."
            {
                if (blk[0] == 0) // empty header == nothing to send
                {
                    uart_putc(ACK);
                    result = 0;
                    break;
                }
                int i = 0;
                for (; blk[i] && i < 100; i++)
                    name[i] = blk[i];
                name[i] = 0;
                int j = i + 1;
                while (blk[j] == ' ')
                    j++;
                for (; blk[j] >= '0' && blk[j] <= '9'; j++)
                    fsize = fsize * 10 + (blk[j] - '0');
                have_header = true;
                expect = 1;
                uart_putc(ACK);
                uart_putc(CRC_C); // request first data block
                continue;
            }
            // data block
            uint32_t copy = len;
            if (fsize && total + copy > fsize) // trim padding past EOF
                copy = fsize - total;
            if (total + copy > bufsz) // never overflow the buffer
                copy = (total < bufsz) ? bufsz - total : 0;
            for (uint32_t i = 0; i < copy; i++)
                buf[total + i] = blk[i];
            total += copy;
            expect++;
            uart_putc(ACK);
        }
        else if ((uint8_t)bn == (uint8_t)(expect - 1))
        {
            uart_putc(ACK); // duplicate (our ACK was lost): re-ACK, don't store
        }
        else
        {
            uart_putc(NAK); // out of sequence
        }
    }

    return result;
}
