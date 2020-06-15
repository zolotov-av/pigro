
#include "stm32f1xx.h"
#include "isr.h"

inline void wait()
{
    for(int i = 0; i < 1000000; i++)
    {
        asm ("nop" ::: "memory");
    }
}

static int main_loop()
{
    // enable GPIOA port
    RCC->APB2ENR = 1 << 2;

    // --- GPIO setup ----
    GPIOA->CRL = (GPIOA->CRL & ~0xF) | 0x2;

    while ( 1 )
    {

        GPIOA->ODR |= 1;
        wait();
        GPIOA->ODR &= ~1;
        wait();
        if ( GPIOA->IDR & 1 ) return 1;
    }

}

void Reset_Handler()
{
    main_loop();
    InfiniteLoop();
}
