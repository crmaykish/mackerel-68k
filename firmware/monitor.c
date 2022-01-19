#define ACIA_DATA 0xC0000
#define ACIA_STATUS 0xC0001
#define ACIA_COMMAND 0xC0002
#define ACIA_CONTROL 0xC0003

#define ACIA_TX_READY 0x10
#define ACIA_RX_READY 0x08

// Get a pointer to a memory address
#define MEM(address) (*(volatile unsigned char *)(address))

void acia_init()
{
    MEM(ACIA_STATUS) = 0;
    MEM(ACIA_COMMAND) = 0x0B;
    MEM(ACIA_CONTROL) = 0b00010000;
}

void putc(unsigned char a)
{
    while ((MEM(ACIA_STATUS) & ACIA_TX_READY) == 0)
    {
    }

    MEM(ACIA_DATA) = a;
}

void puts(unsigned char *s)
{
    unsigned i = 0;

    while (s[i] != 0)
    {
        putc(s[i]);
        i++;
    }
}

unsigned char getc()
{
    while ((MEM(ACIA_STATUS) & ACIA_RX_READY) == 0)
    {
    }

    return MEM(ACIA_DATA);
}

int main()
{
    unsigned char in;

    acia_init();

    puts("Mackerel 68008 Monitor\r\n");

    while (1)
    {
        in = getc();
        putc(in);
    }
}
