// sdramtest.c -- Mackerel-F: verify the 8 MB SDRAM works as system RAM.
//
// The SDRAM is now mapped at 0x000000-0x7FFFFF and IS the system RAM: the stack
// lives in it (reset SP = 0x800000, the top, growing down) and so do .data/.bss.
// This program runs from ROM and reports over UART (115200 8N1, ttyUSB2), pass/fail
// on the LEDs (PASS = all on steady, FAIL = blinking). The fact that it runs at all
// -- function calls, the banner -- already exercises the stack in SDRAM; the tests
// below cover the bulk, in a region disjoint from the running program's memory.

#define GPIO      (*(volatile unsigned char *)0xFFF800)

#define UART_BASE 0xFFF900
#define UART_THR  (*(volatile unsigned char *)(UART_BASE + 0))
#define UART_DLL  (*(volatile unsigned char *)(UART_BASE + 0))
#define UART_DLM  (*(volatile unsigned char *)(UART_BASE + 2))
#define UART_FCR  (*(volatile unsigned char *)(UART_BASE + 4))
#define UART_LCR  (*(volatile unsigned char *)(UART_BASE + 6))
#define UART_LSR  (*(volatile unsigned char *)(UART_BASE + 10))
#define LSR_THRE  0x20

// Program RAM lives low (.data/.bss from 0x400) and high (stack near 0x800000), so
// the test works the span in between -- it never touches the running program.
#define RAM_LO 0x010000UL
#define RAM_HI 0x7F0000UL

static void uart_init(void) {
    UART_LCR = 0x80; UART_DLL = 35; UART_DLM = 0; UART_LCR = 0x03; UART_FCR = 0x07;  // 64.8MHz/(16*35) ~ 115200
}
static void putc_(char c) { while (!(UART_LSR & LSR_THRE)) { } UART_THR = c; }
static void puts_(const char *s) { while (*s) { if (*s == '\n') putc_('\r'); putc_(*s++); } }
static void h4(unsigned v) { putc_("0123456789ABCDEF"[v & 0xF]); }
static void h16(unsigned v) { h4(v >> 12); h4(v >> 8); h4(v >> 4); h4(v); }
static void h32(unsigned long v) { h16((unsigned)(v >> 16)); h16((unsigned)v); }

// word pattern write+verify over n words starting at lo
static unsigned long test_words(unsigned long lo, unsigned long n) {
    volatile unsigned short *p;
    unsigned long i, errs = 0;
    for (i = 0; i < n; i++) { p = (volatile unsigned short *)(lo + i * 2); *p = (unsigned short)(i ^ 0xC3A5); }
    for (i = 0; i < n; i++) { p = (volatile unsigned short *)(lo + i * 2); if (*p != (unsigned short)(i ^ 0xC3A5)) errs++; }
    return errs;
}
// unique marker every 64 KB across the whole span -> catches stuck/aliased address lines
static unsigned long test_addr(void) {
    unsigned long a, errs = 0;
    unsigned n;
    for (a = RAM_LO, n = 0; a < RAM_HI; a += 0x10000UL, n++) *(volatile unsigned short *)a = (unsigned short)(0xA500 | (n & 0xFF));
    for (a = RAM_LO, n = 0; a < RAM_HI; a += 0x10000UL, n++) if (*(volatile unsigned short *)a != (unsigned short)(0xA500 | (n & 0xFF))) errs++;
    return errs;
}
// byte-mask coverage (UDS/LDS individual byte writes)
static unsigned long test_bytes(void) {
    volatile unsigned char *b = (volatile unsigned char *)0x020000UL;
    unsigned i, errs = 0;
    for (i = 0; i < 256; i++) b[i] = (unsigned char)(i ^ 0x5A);
    for (i = 0; i < 256; i++) if (b[i] != (unsigned char)(i ^ 0x5A)) errs++;
    return errs;
}

int main(void) {
    unsigned long total = 0, e;
    volatile unsigned long d;
    int probe;                 // a local -> its address shows where the stack is

    uart_init();
    GPIO = 0x00;
    puts_("\n\nMackerel-F: 8 MB SDRAM as system RAM\n");
    puts_("stack var @ "); h32((unsigned long)&probe);
    puts_("  (near 0x800000 => stack is in SDRAM)\n");

    puts_("words 1M  : "); e = test_words(RAM_LO, 0x80000); total += e;   // 512K words = 1 MB
    puts_(e ? "FAIL errs " : "OK errs "); h32(e); puts_("\n");
    puts_("addr 8M   : "); e = test_addr(); total += e;
    puts_(e ? "FAIL errs " : "OK errs "); h32(e); puts_("\n");
    puts_("bytes 256 : "); e = test_bytes(); total += e;
    puts_(e ? "FAIL errs " : "OK errs "); h32(e); puts_("\n");

    if (total == 0) { puts_("\nALL PASS\n"); GPIO = 0x3F; }
    else            { puts_("\nFAIL total "); h32(total); puts_("\n"); }

    for (;;) { if (total) { GPIO ^= 0x3F; for (d = 0; d < 150000UL; d++) { } } }
    return 0;
}

void _start(void) { main(); }
