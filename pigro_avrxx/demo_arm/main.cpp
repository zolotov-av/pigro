
#include "stm32f1xx.h"
#include "isr.h"
#include <armxx/pin.h>

inline void wait()
{
    for(int i = 0; i < (1000000 /3); i++)
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




    const arm::output_pin led(GPIOA, 0);

    while ( 1 )
    {

        led.enable();
        wait();

        led.disable();
        wait();

    }

}

void Reset_Handler()
{
    main_loop();
    InfiniteLoop();
}
