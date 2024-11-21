#include <stdio.h>
#include "term.h"

void term_set_color(char *color)
{
    duart_puts("\033[");
    duart_puts(color);
    duart_putc('m');
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
    duart_puts(visible ? TERM_CURSOR_SHOW : TERM_CURSOR_HIDE);
}

void term_clear()
{
    term_set_color(TERM_RESET);
    duart_puts("\033[2J\033[H");
}
