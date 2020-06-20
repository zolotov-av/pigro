#ifndef ARMXX_PIN_H
#define ARMXX_PIN_H

#include <cstdint>
#include "stm32f1xx.h"

namespace arm
{

    class input_pin
    {
    private:

        GPIO_TypeDef *port;
        unsigned bit;

    public:

        input_pin() = delete;
        constexpr input_pin(GPIO_TypeDef *p, unsigned b): port(p), bit(b) { }
        constexpr input_pin(const input_pin &) = default;
        constexpr input_pin(input_pin &&) = default;

        bool read() const { return port->IDR & (1 << bit); }
        bool is_on() const { return read(); }
        bool is_off() const { return !read(); }

    };

    class output_pin
    {
    private:

        GPIO_TypeDef *port;
        unsigned bit;

    public:

        constexpr output_pin() = delete;
        constexpr output_pin(GPIO_TypeDef *p, unsigned b): port(p), bit(b) { }
        constexpr output_pin(const output_pin &) = default;
        constexpr output_pin(output_pin &&) = default;

        void enable() const { port->BSRR = (1 << bit); }
        void disable() const { port->BRR = (1 << bit); }

        void write(bool value) const
        {
            if ( value ) enable();
            else disable();
        }

    };

    class pin
    {
    private:

        GPIO_TypeDef *port;
        unsigned bit;

    public:

        constexpr pin() = delete;
        constexpr pin(GPIO_TypeDef *p, unsigned b): port(p), bit(b) { }
        constexpr pin(const pin &) = default;
        constexpr pin(pin &&) = default;

        bool read() const { return port->IDR & (1 << bit); }
        bool is_on() const { return read(); }
        bool is_off() const { return !read(); }

        void enable() const { port->BSRR = (1 << bit); }
        void disable() const { port->BRR = (1 << bit); }

        void write(bool value) const
        {
            if ( value ) enable();
            else disable();
        }

    };


}

#endif // ARMXX_PIN_H
