// Mackerel-F UART + newlib stress / sanity test.
//
// Exercises the new console layer end to end: newlib stdio is routed through
// write() -> console_putc() -> uart_putc() -> the 16550. So every printf here
// proves the whole stack on real hardware. It also stresses a spread of newlib
// library code that lives off in libc (stdio formatting, the malloc heap in
// SDRAM, and string/stdlib helpers), self-checking each with PASS/FAIL and a
// final summary line that's easy to eyeball over serial.
//
// Startup (vectors_f.s -> start.c) sets the vector table, copies .data, calls
// console_init() then newlib_init(), and finally main() -- so this file is just
// main() plus helpers, like any other Mackerel program.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mackerel.h"

#define GPIO 0xFFF800 // 6 LEDs, active low: led = ~gpio[5:0]

static int checks = 0;
static int fails = 0;

static void check(const char *name, int ok)
{
    checks++;
    if (!ok)
        fails++;
    printf("  [%s] %s\r\n", ok ? "PASS" : "FAIL", name);
}

// Show progress / result on the 6 LEDs (value is masked to 6 bits).
static void leds(uint8_t v) { MEM(GPIO) = v; }

int main(void)
{
    leds(0x01);

    printf("\r\n");
    printf("========================================\r\n");
    printf("  %s UART + newlib test\r\n", SYSTEM_NAME);
    printf("  built %s %s, CPU %lu Hz\r\n", __DATE__, __TIME__, (unsigned long)CPU_CLK_HZ);
    printf("========================================\r\n");

    // ---- 1. printf/snprintf format coverage -----------------------------
    // snprintf returns the would-be length and writes into a buffer we then
    // compare byte-for-byte, so a bad formatter is caught, not just printed.
    char buf[80];
    int n;

    n = snprintf(buf, sizeof buf, "%d %u %x %o %c %s", -42, 42u, 0xBEEF, 64, 'Z', "ok");
    check("snprintf int/str formats", n == 20 && strcmp(buf, "-42 42 beef 100 Z ok") == 0);

    n = snprintf(buf, sizeof buf, "%08lX|%+d|%5d|%-5d|", 0xCAFEBABEUL, 7, 3, 3);
    check("snprintf width/flags", strcmp(buf, "CAFEBABE|+7|    3|3    |") == 0);

    // Truncation: snprintf must respect the size and still report full length.
    n = snprintf(buf, 5, "%s", "abcdefgh");
    check("snprintf truncation", n == 8 && strcmp(buf, "abcd") == 0);

    // ---- 2. malloc / free heap in SDRAM ---------------------------------
    leds(0x03);
    {
        // Grab a chunk, fill it, read it back -- exercises sbrk into SDRAM.
        const size_t sz = 64 * 1024;
        unsigned char *p = malloc(sz);
        int ok = (p != NULL);
        if (ok)
        {
            for (size_t i = 0; i < sz; i++)
                p[i] = (unsigned char)(i * 31 + 7);
            for (size_t i = 0; i < sz; i++)
                if (p[i] != (unsigned char)(i * 31 + 7))
                {
                    ok = 0;
                    break;
                }
        }
        free(p);
        check("malloc/fill/verify 64K", ok);
    }
    {
        // Many small alloc/free cycles -- stresses the allocator's free list.
        int ok = 1;
        for (int i = 0; i < 2000; i++)
        {
            void *a = malloc(32 + (i & 63));
            void *b = malloc(128);
            if (!a || !b)
            {
                ok = 0;
                free(a);
                free(b);
                break;
            }
            memset(a, 0xAA, 32);
            free(a);
            free(b);
        }
        check("2000x malloc/free cycles", ok);
    }

    // ---- 3. string / stdlib helpers -------------------------------------
    leds(0x07);
    check("strtol hex", strtol("7B", NULL, 16) == 123);
    check("strtol neg dec", strtol("-1000", NULL, 10) == -1000);
    check("atoi", atoi("2026") == 2026);

    {
        char a[16];
        strcpy(a, "Mack");
        strcat(a, "erel");
        check("strcpy/strcat/strlen", strlen(a) == 8 && strcmp(a, "Mackerel") == 0);
    }
    {
        unsigned char m[32];
        memset(m, 0x5A, sizeof m);
        unsigned char m2[32];
        memcpy(m2, m, sizeof m);
        check("memset/memcpy/memcmp", memcmp(m, m2, sizeof m) == 0 && m2[17] == 0x5A);
    }
    {
        // qsort + bsearch pull in more libc and exercise the callback path.
        int arr[10] = {5, 3, 9, 1, 7, 0, 8, 2, 6, 4};
        extern int cmp_int(const void *, const void *);
        qsort(arr, 10, sizeof(int), cmp_int);
        int ok = 1;
        for (int i = 0; i < 10; i++)
            if (arr[i] != i)
                ok = 0;
        int key = 7;
        int *found = bsearch(&key, arr, 10, sizeof(int), cmp_int);
        check("qsort + bsearch", ok && found && *found == 7);
    }

    // ---- 4. TX throughput burst -----------------------------------------
    // A long contiguous write to shake out any TX-ready/FIFO handling in the
    // console path under sustained load (not just one char at a time).
    leds(0x0F);
    for (int line = 0; line < 16; line++)
    {
        for (int col = 0; col < 64; col++)
            putchar('0' + ((line + col) % 10));
        printf("\r\n");
    }

    // ---- summary --------------------------------------------------------
    printf("========================================\r\n");
    printf("  RESULT: %d checks, %d FAILED\r\n", checks, fails);
    printf("  %s\r\n", fails == 0 ? "ALL PASS" : "*** FAILURES ***");
    printf("========================================\r\n");
    fflush(stdout);

    // LEDs: all on (0x3F) if any failure, all off (0x00) on full pass.
    leds(fails ? 0x3F : 0x00);

    // Return to the caller (the bootloader, when loaded via ymodem + run).
    return fails;
}

// qsort/bsearch comparator (non-static so qsort sees a real function symbol).
int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}
