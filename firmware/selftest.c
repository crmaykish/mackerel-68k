#include "mackerel.h"

int main()
{
    MEM(MFP_DDR) = 0xFF;

    int a = 1;

    while (1)
    {
        MEM(MFP_GPDR) = a;
        // delay(500);

        a++;
    }

    return 0;
}

void _start()
{
    main();
}
