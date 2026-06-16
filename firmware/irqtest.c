// Mackerel-F interrupt test (RAM program, loaded over ymodem then `run`).
//
// End-to-end check of the SoC interrupt path for BOTH sources:
//   * Timer (slot 2) -> IPL 6 -> autovector 30
//   * UART  (slot 1) -> IPL 5 -> autovector 29
// The irq_encoder priority-encodes them onto the fx68k IPL pins; an IACK
// (FC=111) is autovectored via VPAn. Each device is acked in software in its
// ISR -- timer: write STATUS; UART: read RBR (the 16550's read-to-clear).
//
//   Phase 1: timer alone -- masked (IRQ asserts, ISR held off) then unmasked
//            (counter climbs), then disabled (freezes).
//   Phase 2: UART RX by interrupt WITH the timer also running, so both IPL
//            levels are live at once (the timer at 6 preempts the UART at 5).

#include <stdio.h>
#include "mackerel.h"
#include "uart_16550.h"

// Timer (slot 2)
#define TIMER_CTRL   (TIMER_BASE + 0) // [0]=ENABLE, [5:4]=FREQ (3=100 Hz)
#define TIMER_STATUS (TIMER_BASE + 2) // read: [0]=PENDING ; write: ACK

#define TICK_VECTOR (EXCEPTION_AUTOVECTOR + IRQ_NUM_TIMER) // level 6 -> vector 30
#define UART_VECTOR (EXCEPTION_AUTOVECTOR + IRQ_NUM_DUART) // level 5 -> vector 29

volatile uint32_t g_ticks = 0;
volatile uint32_t g_rx_count = 0;
volatile int      g_rx_done = 0; // set when Enter (CR/LF) is received
volatile uint8_t  g_rx[64];

void __attribute__((interrupt)) timer_isr(void)
{
    MEM(TIMER_STATUS) = 0; // ack: drop the level-6 IRQ before RTE
    g_ticks++;
}

void __attribute__((interrupt)) uart_isr(void)
{
    // Drain the RX FIFO; reading RBR clears the RDA interrupt (16550 read-to-clear).
    while (MEM(UART_LSR) & LSR_DR)
    {
        uint8_t b = MEM(UART_RBR);
        if (g_rx_count < sizeof(g_rx))
            g_rx[g_rx_count] = b;
        g_rx_count++;
        if (b == '\r' || b == '\n')
            g_rx_done = 1;
    }
}

int main(void)
{
    printf("\r\n=== Mackerel-F interrupt test (timer + UART) ===\r\n");
    printf("timer: level %d -> vector %d   uart: level %d -> vector %d\r\n",
           IRQ_NUM_TIMER, TICK_VECTOR, IRQ_NUM_DUART, UART_VECTOR);

    set_interrupts(false);
    set_exception_handler(TICK_VECTOR, timer_isr);
    set_exception_handler(UART_VECTOR, uart_isr);

    // ---- Phase 1: timer IRQ (level 6) -----------------------------------
    g_ticks = 0;
    MEM(TIMER_CTRL) = 0x01 | (3 << 4); // enable, 100 Hz
    for (volatile uint32_t i = 0; i < 2000000UL; i++) {}
    uint8_t pending = MEM(TIMER_STATUS) & 1;
    printf("\r\n[timer] masked:   pending=%u ticks=%lu   (expect pending=1, ticks=0)\r\n",
           pending, (unsigned long)g_ticks);

    set_interrupts(true);
    uint32_t prev = g_ticks;
    int climbing = 1;
    for (int s = 0; s < 3; s++)
    {
        sleep_ms(500);
        uint32_t now = g_ticks;
        printf("[timer] unmasked: ticks=%lu (+%lu)\r\n",
               (unsigned long)now, (unsigned long)(now - prev));
        if (now <= prev)
            climbing = 0;
        prev = now;
    }
    set_interrupts(false);
    MEM(TIMER_CTRL) = 0x00;
    uint32_t frozen = g_ticks;
    for (volatile uint32_t i = 0; i < 2000000UL; i++) {}
    int stopped = (g_ticks == frozen);
    printf("[timer] disabled: ticks=%lu, after wait=%lu   (%s)\r\n",
           (unsigned long)frozen, (unsigned long)g_ticks,
           stopped ? "frozen OK" : "STILL TICKING?!");
    int pass_timer = (pending == 1) && climbing && stopped && (g_ticks > 0);

    // ---- Phase 2: UART RX IRQ (level 5), timer also live (level 6) -------
    while (MEM(UART_LSR) & LSR_DR) // flush any stale RX bytes
        (void)MEM(UART_RBR);
    g_rx_count = 0;
    g_rx_done = 0;
    g_ticks = 0;

    MEM(TIMER_CTRL) = 0x01 | (3 << 4); // timer back on (level 6), runs alongside
    MEM(UART_IER) = IER_RDA;           // enable RX-data-available interrupt (level 5)
    set_interrupts(true);

    printf("\r\n[uart] type characters then Enter (RX is interrupt-driven; timer ticking too):\r\n");

    // Wait for Enter via the UART ISR (NOT polling the UART here -- the ISR owns RX).
    uint32_t guard = 0;
    while (!g_rx_done && guard < 60000000UL)
        guard++;

    MEM(UART_IER) = 0x00; // back to a quiet UART for the bootloader's polled console
    MEM(TIMER_CTRL) = 0x00;
    set_interrupts(false);

    printf("[uart] received %lu byte(s) by interrupt: \"", (unsigned long)g_rx_count);
    for (uint32_t i = 0; i < g_rx_count && i < sizeof(g_rx); i++)
    {
        uint8_t b = g_rx[i];
        if (b >= 0x20 && b < 0x7F)
            putchar(b);
        else
            printf("\\x%02X", b);
    }
    printf("\"\r\n");
    printf("[uart] timer kept ticking during RX: %lu ticks (proves both IPL levels live)\r\n",
           (unsigned long)g_ticks);
    int pass_uart = g_rx_done && (g_rx_count > 0) && (g_ticks > 0);

    int pass = pass_timer && pass_uart;
    printf("\r\n=== %s ===\r\n",
           pass ? "PASS: timer + UART interrupts work end to end"
                : "FAIL");
    return pass ? 0 : 1;
}
