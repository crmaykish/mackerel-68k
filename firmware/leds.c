// Baremetal UART echo test: read a byte from the UART, echo it back out.
// The echo is done in C (on the 68k) -- read RBR, write THR -- NOT via the
// 16550's internal loopback. Proves the RX path and the LSR status bits.

#define GPIO_ADDR 0xFFF800

#define UART_BASE 0xFFF900
#define UART_THR  (*(volatile unsigned char *)(UART_BASE + 0))   // write: TX holding
#define UART_RBR  (*(volatile unsigned char *)(UART_BASE + 0))   // read:  RX buffer
#define UART_DLL  (*(volatile unsigned char *)(UART_BASE + 0))   // DLAB=1: divisor low
#define UART_DLM  (*(volatile unsigned char *)(UART_BASE + 2))   // DLAB=1: divisor high
#define UART_FCR  (*(volatile unsigned char *)(UART_BASE + 4))   // write: FIFO control
#define UART_LCR  (*(volatile unsigned char *)(UART_BASE + 6))
#define UART_LSR  (*(volatile unsigned char *)(UART_BASE + 10))

#define LSR_DR    0x01   // data ready: an RX byte is waiting in RBR/FIFO
#define LSR_THRE  0x20   // TX holding register empty: ok to write THR

#define MEM(address) (*(volatile unsigned char *)(address))

static void uart_init(void)
{
    UART_LCR = 0x80;   // DLAB=1 -> divisor latches visible
    UART_DLL = 41;     // 75.6MHz / (16*41) = 115244  (~115200, +0.04%)
    UART_DLM = 0;
    UART_LCR = 0x03;   // DLAB=0, 8 data bits / no parity / 1 stop (8N1)
    UART_FCR = 0x07;   // enable + clear RX & TX FIFOs
}

static void uart_putc(char c)
{
    while (!(UART_LSR & LSR_THRE))   // wait until the TX holding reg is empty
        ;
    UART_THR = c;
}

static unsigned char uart_getc(void)
{
    while (!(UART_LSR & LSR_DR))      // wait until a byte has been received
        ;
    return UART_RBR;
}

static void uart_puts(const char *s)
{
    while (*s)
        uart_putc(*s++);
}

int main(void)
{
    uart_init();
    uart_puts("\r\nMackerel-F UART echo ready\r\n");

    while (1)
    {
        unsigned char c = uart_getc();   // blocks on LSR DR, then reads RBR
        uart_putc(c);                    // echo the byte back out (echo in C)
        if (c == '\r')                   // make Enter behave on the terminal
            uart_putc('\n');
        MEM(GPIO_ADDR) = c;              // show the last received byte on the LEDs
    }

    return 0;
}

void _start(void)
{
    main();
}
