#ifndef AVRXX_H
#define AVRXX_H

#include <stdint.h>

namespace avr
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

    inline void sleep_loop()
    {
        while ( true ) sleep();
    }

    template <typename T>
    class pin
    {
    private:

        volatile T &addr;
        int bit;

    public:

        constexpr pin(T &a, int b): addr(a), bit(b) { }

        bool value() const
        {
            return addr & bit;
        }

        void set(bool x = true) const
        {
            if ( x ) addr |= 1 << bit;
            else addr &= ~(1 << bit);
        }

        void clear() const
        {
            addr &= ~(1 << bit);
        }

    };

    template <typename T>
    class bitfield
    {
    private:

        T &addr;
        volatile T &vaddr;
        int offset;
        int length;

        static constexpr T mask(int len)
        {
            return (1 << len) - 1;
        }

    public:

        constexpr bitfield(volatile T &a, int o, int l): addr((T &)a), vaddr(a), offset(o), length(l) { }

        T extract(T data) const
        {
            return (data >> offset) & mask(length);
        }

        template <typename T2>
        T replace(T data, T2 value) const
        {
            const T clear_mask = (~mask(length)) << offset;
            const T set_mask = (value & mask(length)) << offset;
            return (data & clear_mask) | set_mask;
        }

        T value() const
        {
            return extract(addr);
        }

        template <typename T2>
        void set(T2 value) const
        {
            addr = replace(addr, value);
        }

        T read() const
        {
            return extract(vaddr);
        }

        template <typename T2>
        void write(T2 value) const
        {
            vaddr = replace(addr, value);
        }

    };

    template <typename T = uint8_t>
    class ioreg
    {
    private:

        T &addr;
        volatile T &vaddr;

    public:

        constexpr ioreg(volatile T &a): addr((T &)a), vaddr(a) { }
        constexpr ioreg(const ioreg &) = default;

        /**
         * Непосредственная запись в порт
         *
         * Используется для "двойных" регистров и специальной логики,
         * например регистры UBRRH/UCSRC, OSCCAL/OCDR
         */
        void raw_write(uint8_t value) const
        {
            const uint8_t port = reinterpret_cast<int>(&addr) - 0x20;
            // здесь не нужен ни "memory" ни __volatile__
            __asm__  ( "out %1, %0" :: "r"(value), "I"(port) );
        }

        void clear() const
        {
            addr = 0;
        }

        bool pin(int offset) const
        {
            return addr & (1 << offset);
        }

        void setPin(int offset, bool value) const
        {
            if ( value ) addr |= (1 << offset);
            else addr &= ~(1 << offset);
        }

        bool read_pin(int offset) const
        {
            return vaddr & (1 << offset);
        }


        void write_pin(int offset, bool value) const
        {
            if ( value ) addr |= (1 << offset);
            else vaddr &= ~(1 << offset);
        }

        T value() const
        {
            return addr;
        }

        T value(int offset, int length) const
        {
            return bitfield<T>(vaddr, offset, length).value();
        }

        void set(T value) const
        {
            addr = value;
        }

        template <typename T2>
        void set(int offset, int length, T2 value) const
        {
            bitfield<T>(vaddr, offset, length).set(value);
        }

        T read() const
        {
            return vaddr;
        }

        T read(int offset, int length) const
        {
            return bitfield<T>(vaddr, offset, length).read();
        }

        template <typename T2>
        void write(int offset, int length, T2 value) const
        {
            bitfield<T>(vaddr, offset, length).write(value);
        }

        void write(T value) const
        {
            vaddr = value;
        }

    };

}

#endif // AVRXX_H
