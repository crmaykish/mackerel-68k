#include "newlib_init.h"

extern int main();

void _start()
{
    newlib_init();
    main();
}
