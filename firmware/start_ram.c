#include <sys/reent.h>
#include <string.h>
#include <stdio.h>

extern char _sbss[], _ebss[];

extern struct _reent *_impure_ptr;
extern struct _reent impure_data;

extern void __libc_init_array(void);

extern int main();

static void early_newlib_init(void)
{
    // Set up the global reentrancy struct
    _impure_ptr = &impure_data;
    _REENT_INIT_PTR(_impure_ptr);

    // Run newlib’s C init arrays
    __libc_init_array();
}

void _start()
{
    // Clear the BSS section
    for (char *p = _sbss; p < _ebss; ++p)
    {
        *p = 0;
    }

    early_newlib_init();
    setvbuf(stdout, NULL, _IONBF, 0); // disable stdout buffering

    // Call main
    main();
}
