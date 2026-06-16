#include <stdio.h>
#include "term.h"
#include "console.h"

void term_set_color(char *color)
{
    console_puts("\033[");
    console_puts(color);
    console_putc('m');
}

void term_cursor_move(term_cursor_dir_e dir, uint8_t steps)
{
    printf("\033[%d%c", steps, dir);
}

void term_cursor_set_x(uint8_t x)
{
    printf("\033[%dG", x);
}

void term_cursor_set_pos(uint8_t x, uint8_t y)
{
    printf("\033[%d;%dH", x, y);
}

void term_cursor_set_vis(bool visible)
{
    console_puts(visible ? TERM_CURSOR_SHOW : TERM_CURSOR_HIDE);
}

void term_clear()
{
    term_set_color(TERM_RESET);
    console_puts("\033[2J\033[H");
}

// Draw a fancy progress bar with percentage
void term_progress_bar(int pct)
{
    int filled = (pct * TERM_BAR_WIDTH) / 100;

    console_putc('\r');
    console_putc('[');
    for (int i = 0; i < TERM_BAR_WIDTH; i++)
        console_putc(i < filled ? '#' : '-');
    console_putc(']');
    console_putc(' ');

    // Right-align the percentage in a 3-character field so it doesn't jitter.
    if (pct >= 100)
    {
        console_putc('1');
        console_putc('0');
        console_putc('0');
    }
    else
    {
        console_putc(' ');
        console_putc(pct >= 10 ? '0' + pct / 10 : ' ');
        console_putc('0' + pct % 10);
    }
    console_putc('%');
}
