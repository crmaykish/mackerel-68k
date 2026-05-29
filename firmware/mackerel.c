#include <stdio.h>
#include "mackerel.h"
#include "term.h"

void set_interrupts(bool enabled)
{
    if (enabled)
    {
        // Set minimum interrupt level to 0 in the status register
        asm("and.w #0xF8FF, %sr");
    }
    else
    {
        // Set minimum interrupt level to 7, i.e. only non-maskable interrupts enabled
        asm("or.w #0x700, %sr");
    }
}

void set_exception_handler(unsigned char exception_number, void (*exception_handler)())
{
#ifdef MACKEREL_30
    MEM32(get_vbr() + exception_number * 4) = (uint32_t)exception_handler;
#else
    MEM32(exception_number * 4) = (uint32_t)exception_handler;
#endif
}

void duart_init(void)
{
    MEM(DUART1_IMR) = 0x00;        // Mask all interrupts
    MEM(DUART1_OPCR) = 0x00;       // OP0/OP1 = RTSA//RTSB/, OP2-OP7 = general-purpose GPIO
    MEM(DUART1_MR1B) = 0b00010011; // No Rx RTS, No Parity, 8 bits per character
    MEM(DUART1_MR2B) = 0b00000111; // Channel mode normal, No Tx RTS, No CTS, stop bit length 1
    MEM(DUART1_ACR) = 0x80;        // Baudrate set 2
    MEM(DUART1_CRB) = 0x80;        // Set Rx extended bit
    MEM(DUART1_CRB) = 0xA0;        // Set Tx extended bit
    MEM(DUART1_CSRB) = 0x88;       // 115200 baud
    MEM(DUART1_CRB) = 0b0101;      // Enable Tx/Rx
}

void duart_putc(char c)
{
    while ((MEM(DUART1_SRB) & 0b00000100) == 0)
    {
    }

    MEM(DUART1_TBB) = c;

    if (c == 0x0A)
    {
        duart_putc(0x0D);
    }
}

char duart_getc(void)
{
    while ((MEM(DUART1_SRB) & 0b00000001) == 0)
    {
    }

    return MEM(DUART1_RBB);
}

void duart_puts(const char *s)
{
    unsigned i = 0;

    while (s[i] != 0)
    {
        duart_putc(s[i]);
        i++;
    }
}

void print_string_bin(char *str, uint8_t max)
{
    uint8_t i = 0;

    while (i < max)
    {
        if (str[i] >= 32 && str[i] < 127)
        {
            duart_putc(str[i]);
        }
        else
        {
            duart_putc('.');
        }

        i++;
    }
}

void memdump(uint32_t address, uint32_t bytes)
{
    uint8_t line[16];
    uint32_t i = 0;

    while (i < bytes)
    {
        uint32_t line_start = i;
        uint32_t line_len = 0;

        // Buffer one line before printing anything
        while (line_len < 16 && i < bytes)
        {
            line[line_len++] = MEM(address + i++);
        }

        printf("%08lX  ", address + line_start);

        for (uint32_t j = 0; j < line_len; j++)
        {
            if (line[j] == 0)
                term_set_color(TERM_FG_GREY);

            printf("%02X ", line[j]);

            term_set_color(TERM_RESET);

            if (j == 7)
                duart_putc(' ');
        }

        printf(" |");
        print_string_bin((char *)line, line_len);
        printf("|\r\n");
    }
}

uint16_t bswap16(uint16_t value)
{
    return (value >> 8) | (value << 8);
}

uint32_t bswap32(uint32_t value)
{
    return ((value >> 24) & 0xFF) |
           ((value >> 8) & 0xFF00) |
           ((value << 8) & 0xFF0000) |
           ((value << 24) & 0xFF000000);
}

void sleep_us(uint32_t us)
{
    uint32_t clocks = us * (CPU_CLK_HZ / 1000000UL);
    uint32_t count = (clocks + SLEEP_CYCLES_PER_LOOP - 1) / SLEEP_CYCLES_PER_LOOP;
    if (count == 0) count = 1;
    for (volatile uint32_t i = count; i > 0; i--);
}

void sleep_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++)
        sleep_us(1000);
}

void delay(int time)
{
    for (int delay = 0; delay < time; delay++)
        __asm__ __volatile__("");
}

#ifdef MACKEREL_30
void set_vbr(unsigned int vbr_val)
{
    asm volatile("movec	%0,%%vbr"
                 : : "d"(vbr_val));
}

unsigned int get_vbr()
{
    unsigned int vbr_value;
    asm volatile(
        "movec %%vbr, %0"
        : "=d"(vbr_value));
    return vbr_value;
}

#endif
