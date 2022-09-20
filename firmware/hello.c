#include "mackerel.h"

int main()
{
    mputs("Echo test:\r\n");

    while (1)
    {
        char a = mgetc();
        mputc(a);
    }

    return 0;
}
