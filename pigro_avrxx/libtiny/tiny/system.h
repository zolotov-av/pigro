#ifndef TINY_SYSTEM_H
#define TINY_SYSTEM_H

#ifdef AVR
#include <tiny/system_avr.h>
#elif defined(__ARM_ARCH)
#include <tiny/system_arm.h>
#else
#error "unknown architecture"
#endif

namespace tiny
{

    class interrupt_lock
    {
    public:

        interrupt_lock()
        {
            interrupt_disable();
        }

        ~interrupt_lock()
        {
            interrupt_enable();
        }
    };

}

#endif // TINY_SYSTEM_H
