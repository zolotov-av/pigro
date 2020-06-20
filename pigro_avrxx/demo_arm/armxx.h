#ifndef TINY_ARMXX_H
#define TINY_ARMXX_H

#include <cstdint>
#include "stm32f1xx.h"
#include <armxx/bitband.h>

namespace arm
{

    class pin
    {
    private:

        GPIO_TypeDef *port;
        int bit;

    public:

        constexpr pin(GPIO_TypeDef *p, int b): port(p), bit(b) { }

        bool direct_value() const
        {
            return port->IDR & (1 << bit);
        }

        void direct_set(bool x = true) const
        {
            if ( x ) port->ODR |= 1 << bit;
            else port->ODR &= ~(1 << bit);
        }

        bool bitbang_value() const
        {
            return bitband_read(port->IDR, bit);
        }

        void bitbang_set(bool value) const
        {
            bitband_write(port->ODR, bit, value);
        }

        bool value() const
        {
            return direct_value();
        }

        void set(bool x) const
        {
            return direct_set(x);
        }

        void clear() const
        {
            set(false);
        }

    };

}

#endif // TINY_ARMXX_H
