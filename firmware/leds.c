// Baremetal self-contained simple LED/GPIO test

#define GPIO_ADDR 0xFFF800
#define DELAY_CNT 100000

#define MEM(address) (*(volatile char *)(address))

void delay(int time)
{
    for (int delay = 0; delay < time; delay++)
        __asm__ __volatile__("");
}

int main()
{
    while (1)
    {
        MEM(GPIO_ADDR) = 0xAA;
        delay(DELAY_CNT);
        MEM(GPIO_ADDR) = 0x55;
        delay(DELAY_CNT);
    }

    return 0;
}

void _start()
{
    main();
}
