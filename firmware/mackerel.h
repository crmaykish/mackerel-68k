#ifndef _MACKEREL_H
#define _MACKEREL_H

#include <stdbool.h>
#include <stdint.h>
#include "mem.h"

#define EXCEPTION_AUTOVECTOR 24
#define EXCEPTION_USER 64

#define IRQ_NUM_IDE 3
#define IRQ_NUM_DUART 5
#define IRQ_NUM_TIMER 6

// UART_BASE is the console UART's base address (the per-chip register map lives
// in the driver header: uart_xr68c681.h or uart_16550.h).
#ifdef MACKEREL_30
#define SYSTEM_NAME "Mackerel-30"
#define UART_BASE 0xF0000000
#define IDE_BASE     0xF0010000
#define IDE_CTL_BASE 0xF0020000
#define PROGRAM_START 0x1000
#define CPU_CLK_HZ 24000000UL
// 68030 with I-cache only
#define SLEEP_CYCLES_PER_LOOP 10
#elif MACKEREL_10
#define SYSTEM_NAME "Mackerel-10"
#define UART_BASE 0xFF8000
#define IDE_BASE 0xFFC000
#define IDE_CTL_BASE 0xFF4000
#define PROGRAM_START 0x400
#define CPU_CLK_HZ 10000000UL
#define SLEEP_CYCLES_PER_LOOP 40
#elif MACKEREL_08
#define SYSTEM_NAME "Mackerel-08"
#define UART_BASE 0x3FC000
// IDE is not supported on Mackerel-08
#define IDE_BASE 0xFFFFFF
#define PROGRAM_START 0x400
#define CPU_CLK_HZ 12000000UL
#define SLEEP_CYCLES_PER_LOOP 80
#endif

// Enable/disable CPU interrupts
void set_interrupts(bool enabled);

// Assign a handler function to an exception number (0x00 - 0xFF)
void set_exception_handler(unsigned char exception_number, void (*exception_handler)());

// Utils
void memdump(uint32_t address, uint32_t bytes);

uint16_t bswap16(uint16_t value);
uint32_t bswap32(uint32_t value);

// Timing
void sleep_us(uint32_t us);
void sleep_ms(uint32_t ms);
void delay(int time);

#ifdef MACKEREL_30
void set_vbr(unsigned int vbr_val);
unsigned int get_vbr();

#define CACR_ENABLE_ICACHE   0x0001
#define CACR_FREEZE_ICACHE   0x0002
#define CACR_CLEAR_ICACHE    0x000
#define CACR_ENABLE_DCACHE   0x0100
#define CACR_FREEZE_DCACHE   0x0200
#define CACR_CLEAR_DCACHE    0x0800

#endif

#endif
