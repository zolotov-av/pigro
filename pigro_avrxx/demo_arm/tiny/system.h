#ifndef TINY_SYSTEM_H
#define TINY_SYSTEM_H

#include <tiny/system_arm.h>

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
