#include <sys/stat.h>
#include <sys/reent.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include "mackerel.h"

// Implement enough syscalls for newlib to function

extern char _end;   // from linker, start of heap

struct _reent impure_data;

void _init(void) {}
void _fini(void) {}

ssize_t write(int fd, const void *buf, size_t count)
{
    if (fd != 1 && fd != 2) return -1;   // only stdout/stderr
    const char *p = buf;
    for (size_t i = 0; i < count; i++) {
        duart_putc(p[i]);
    }
    return (ssize_t)count;
}

ssize_t read(int fd, void *buf, size_t count)
{
    return 0;   // TODO
}

void *sbrk(ptrdiff_t incr)
{
    static char *heap_end;
    char *prev_heap_end;

    if (heap_end == 0)
        heap_end = &_end;

    prev_heap_end = heap_end;
    heap_end += incr;

    return (void *)prev_heap_end;
}

int close(int fd)
{
    errno = EBADF;
    return -1;
}

int fstat(int fd, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}

int isatty(int fd)
{
    if (fd == 1 || fd == 2) return 1;
    return 0;
}

off_t lseek(int fd, off_t offset, int whence)
{
    return 0;
}
