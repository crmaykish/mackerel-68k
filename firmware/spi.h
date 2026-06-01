#ifndef SPI_H
#define SPI_H

#include "mackerel.h"

// SPI output pins
#define SCLK (3)
#define MOSI (2) // DI on SD card
#define CS0     (4)
#define CS1     (5)
#define CS2     (6)
#define CS_ENC  CS0   /* ENC28J60 Ethernet chip select (OP4) */

// SPI input pins
#define MISO (4) // DO on SD card

#define LED (7)

#define SPI_EMPTY 0xFF

#define SPI_RETRY_LIMIT 10

// ACMD41 can take up to ~1s for the card to finish internal init
#define SD_INIT_RETRY_LIMIT 1000

// Set up the GPIO pins needed for SPI
void spi_init(uint8_t cs);

// Send and receive a single byte over SPI (asserts/deasserts CS around transfer)
uint8_t spi_transfer(uint8_t cs, uint8_t b);

// Explicit chip-select control (active low)
void spi_cs_low(uint8_t cs);
void spi_cs_high(uint8_t cs);

// Transfer one byte without touching CS; SCLK starts and ends low
uint8_t spi_byte(uint8_t b);

void spi_loop_clk();

// Receive one byte (caller holds CS, MOSI high, SCLK starts and ends low).
//
// The Mackerel-08 version below is hand-tuned inline assembly. It keeps the
// three DUART register addresses in address registers so each GPIO access is a
// short register-indirect move rather than an absolute-long one, roughly
// halving the bus cycles per bit on the 68008's 8-bit bus. always_inline lets
// those address loads hoist out of the caller's 512-byte receive loop. Behaves
// identically to the portable C version.
//
// This optimization came from Claude analysis of the code.
#ifdef MACKEREL_08
static inline __attribute__((always_inline)) uint8_t spi_recv(void)
{
    uint8_t r, tmp;
    asm volatile (
        "moveq   #0,  %[r]\n\t"

        // 8 bits, MSB first. Each line: SCLK high, sample MISO (IP bit 4),
        // SCLK low, then shift the bit into the result via the X flag.
        "move.b %[sclk],(%[clkhi])\n\t" "move.b (%[miso]),%[tmp]\n\t" "move.b %[sclk],(%[clklo])\n\t" "lsr.b #5,%[tmp]\n\t" "roxl.b #1,%[r]\n\t"
        "move.b %[sclk],(%[clkhi])\n\t" "move.b (%[miso]),%[tmp]\n\t" "move.b %[sclk],(%[clklo])\n\t" "lsr.b #5,%[tmp]\n\t" "roxl.b #1,%[r]\n\t"
        "move.b %[sclk],(%[clkhi])\n\t" "move.b (%[miso]),%[tmp]\n\t" "move.b %[sclk],(%[clklo])\n\t" "lsr.b #5,%[tmp]\n\t" "roxl.b #1,%[r]\n\t"
        "move.b %[sclk],(%[clkhi])\n\t" "move.b (%[miso]),%[tmp]\n\t" "move.b %[sclk],(%[clklo])\n\t" "lsr.b #5,%[tmp]\n\t" "roxl.b #1,%[r]\n\t"
        "move.b %[sclk],(%[clkhi])\n\t" "move.b (%[miso]),%[tmp]\n\t" "move.b %[sclk],(%[clklo])\n\t" "lsr.b #5,%[tmp]\n\t" "roxl.b #1,%[r]\n\t"
        "move.b %[sclk],(%[clkhi])\n\t" "move.b (%[miso]),%[tmp]\n\t" "move.b %[sclk],(%[clklo])\n\t" "lsr.b #5,%[tmp]\n\t" "roxl.b #1,%[r]\n\t"
        "move.b %[sclk],(%[clkhi])\n\t" "move.b (%[miso]),%[tmp]\n\t" "move.b %[sclk],(%[clklo])\n\t" "lsr.b #5,%[tmp]\n\t" "roxl.b #1,%[r]\n\t"
        "move.b %[sclk],(%[clkhi])\n\t" "move.b (%[miso]),%[tmp]\n\t" "move.b %[sclk],(%[clklo])\n\t" "lsr.b #5,%[tmp]\n\t" "roxl.b #1,%[r]\n\t"
        : [r]     "=&d" (r),
          [tmp]   "=&d" (tmp)
        : [clkhi] "a"   ((volatile uint8_t *)DUART1_OPR_RESET),
          [clklo] "a"   ((volatile uint8_t *)DUART1_OPR),
          [miso]  "a"   ((volatile uint8_t *)DUART1_IP),
          [sclk]  "d"   ((uint8_t)8)
        : "cc"
    );
    return r;
}
#else
uint8_t spi_recv(void);
#endif

#endif
