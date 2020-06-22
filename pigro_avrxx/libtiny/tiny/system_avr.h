#ifndef SYSTEM_AVR_H
#define SYSTEM_AVR_H

#include <avr/io.h>

namespace tiny
{

    inline void interrupt_enable()
    {
        __asm__ __volatile__ ("sei" ::: "memory");
    }

    inline void interrupt_disable()
    {
        __asm__ __volatile__ ("cli" ::: "memory");
    }

    inline void sleep()
    {
        __asm__ __volatile__ ("sleep" ::: "memory");
    }

}

#endif // SYSTEM_AVR_H
