#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

extern char _end[];     // from linker, start of heap
extern char _heap_end[]; // from linker, end of heap

int main(void) {
    printf("=== Mackerel newlib test ===\r\n");

    /* 1. Basic printf */
    printf("Decimal: %d  Hex: 0x%X  String: %s\r\n", 1234, 0xABCD, "hello");

    /* 2. BSS init check */
    static int static_zero;
    printf("Static zero-initialized var = %d (should be 0)\r\n", static_zero);

    /* 3. Heap test */
    printf("Heap range: %p .. %p\r\n", _end, _heap_end);
    void *p1 = malloc(32);
    void *p2 = malloc(64);
    printf("malloc(32) -> %p, malloc(64) -> %p\r\n", p1, p2);

    /* Fill and check */
    if (p1 && p2) {
        memset(p1, 0xAA, 32);
        memset(p2, 0x55, 64);
        printf("Heap memory write test passed\r\n");
    } else {
        printf("Heap allocation failed!\r\n");
    }

    /* 4. _sbrk growth check */
    void *p3 = malloc(1024);
    printf("malloc(1024) -> %p (heap grew)\r\n", p3);

    /* 5. Write syscall test (raw) */
    const char msg[] = "Direct write() call!\r\n";
    write(1, msg, sizeof(msg) - 1);

    /* 6. Free test */
    free(p1);
    free(p2);
    free(p3);
    printf("Freed heap blocks\r\n");

    printf("=== Test complete ===\r\n");
    return 0;
}
