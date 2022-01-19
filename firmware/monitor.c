#include "mackerel.h"

int main()
{
    char input[40];

    mack_acia_init();

    puts("Mackerel 68k\r\n");

    // Echo each line back as it's typed in
    while (1)
    {
        puts("> ");
        readline(input);

        puts("\r\nLine: ");

        puts(input);

        puts("\r\n");
    }
}
