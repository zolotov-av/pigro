
#include "stm32f1xx.h"
#include "isr.h"
#include <tiny/system.h>
#include <tiny/io.h>
#include <armxx/pin.h>

inline void wait()
{
    for(int i = 0; i < (1000000 /3); i++)
    {
        asm ("nop" ::: "memory");
    }
}

class led_pin
{
private:

   arm::output_pin pin {GPIOA, 1};

public:

    void enable() const { pin.disable(); }
    void disable() const { pin.enable(); }
    void set(bool value) const { pin.write(!value); }

};

static bool state;

void SysTick_Handler()
{
    state = !state;
    led_pin().set(state);
}

constexpr uint32_t GPIO_CR_DEFAULT = 0x44444444;

static int main_loop()
{
    // enable GPIOA port
    RCC->APB2ENR = tiny::makebits(2, 14);

    // --- GPIO setup ----
    GPIOA->CRL = (GPIO_CR_DEFAULT & ~0xF0) | 0x20;
    GPIOA->CRH = (GPIO_CR_DEFAULT & ~(0xFF<<4)) | (0b1010 << 4) | (0b1000 << 8);

    SysTick->LOAD = 1000000 * 4;
    SysTick->VAL = 0;
    SysTick->CTRL = tiny::makebits(SysTick_CTRL_CLKSOURCE_Pos, SysTick_CTRL_TICKINT_Pos, SysTick_CTRL_ENABLE_Pos);

    tiny::interrupt_enable();
    NVIC_EnableIRQ(USART1_IRQn);

    state = true;

    // NOTE: в текущей версии программатора, после выполнения команд процессор
    // оставляется в выключенном состоянии, поэтому код может не работать,
    // для работы отладчную плату желательно запитать отдельно, не от программатора
    //
    // TODO: сделать команду запуска микроконтроллера или после прошивки автоматически
    // переводить в активное состояние.
    // Я не помню почему так сделано и я не знаю, когда у меня будет время с этим
    // разобраться, поддержка STM32 до сих пор является экспериментальной и что-нибудь
    // может не работать.

    led_pin().set(state);


    while ( true )
    {
        tiny::sleep();
        led_pin().set(state);
    }
}

extern "C" uint32_t _sidata;
extern "C" uint32_t _sdata;
extern "C" uint32_t _edata;
extern "C" uint32_t _sbss;
extern "C" uint32_t _ebss;

extern "C" void init_data()
{
    const uint32_t size = &_edata - &_sdata;
    for(uint32_t i = 0; i < size; i++)
    {
        (&_sdata)[i] = (&_sidata)[i];
    }

    for(uint32_t *dst = &_sbss; dst < &_ebss; dst++)
    {
        *dst = 0;
    }
}

void Reset_Handler()
{
    init_data();
    main_loop();
    InfiniteLoop();
}
