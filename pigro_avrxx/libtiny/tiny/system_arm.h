#ifndef TINY_SYSTEM_ARM_H
#define TINY_SYSTEM_ARM_H

#include <stm32f1xx.h>

namespace tiny
{

    inline void interrupt_enable()
    {
        __enable_irq();
    }

    inline void interrupt_disable()
    {
        __disable_irq();
    }

    inline void sleep()
    {
        __WFI();
    }

}

#endif // TINY_SYSTEM_ARM_H
