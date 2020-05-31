#ifndef AVRXX_H
#define AVRXX_H

#include <stdint.h>

namespace avr
{

    constexpr auto makebits()
    {
        return 0;
    }

    template<typename T>
    constexpr T makebits(T first) {
        return (1 << first);
    }

    template<typename T, typename... Args>
    constexpr T makebits(T first, Args... args) {
        return (1 << first) | makebits(args...);
    }

    template <typename T>
    void setreg(volatile T &reg)
    {
        reg = 0;
    }

    template <typename T>
    void setreg(volatile T &reg, int bit)
    {
        reg = (1 << bit);
    }

    template <typename T, typename... Bits>
    void setreg(volatile T &reg, Bits... bits)
    {
        reg = makebits(bits...);
    }

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

    template <typename T> class cached_io;
    template <typename T> class forced_io;
    template <typename T> class raw_io;

    template <typename T>
    class cached_io
    {
    private:

        T &var;

    public:

        T &ref() const { return var; }

        constexpr cached_io(volatile T &r): var((T &)r) { }
        constexpr cached_io(const cached_io &) = default;
        constexpr cached_io(cached_io &&) = delete;
        template <class IO> constexpr cached_io(const IO &io): var(io.ref) { }

        T read() const
        {
            return var;
        }

        void write(T value) const
        {
            var = value;
        }

    };

    template <typename T>
    class forced_io
    {
    private:

        volatile T &var;

    public:

        volatile T &ref() const { return var; }

        constexpr forced_io(volatile T &r): var(r) { }
        constexpr forced_io(const forced_io &) = default;
        constexpr forced_io(forced_io &&) = delete;
        template <class IO> constexpr forced_io(const IO &io): var(io.ref) { }

        T read() const
        {
            return var;
        }

        void write(T value) const
        {
            var = value;
        }

    };

    template <typename T>
    class raw_io
    {
    private:

        volatile T &var;

    public:

        volatile T &ref() const { return var; }

        constexpr raw_io(volatile T &r): var(r) { }
        constexpr raw_io(const raw_io &) = default;
        constexpr raw_io(raw_io &&) = delete;
        template <class IO> constexpr raw_io(const IO &io): var(io.ref) { }

        T read() const
        {
            const uint8_t port = reinterpret_cast<int>(&var) - 0x20;
            // здесь не нужен ни "memory" ни __volatile__
            uint8_t value;
            __asm__  ( "in %0, %1" : "=r"(value) : "I"(port) );
            return value;

        }

        void write(T value)
        {
            const uint8_t port = reinterpret_cast<int>(&var) - 0x20;
            // здесь не нужен ни "memory" ни __volatile__
            __asm__  ( "out %1, %0" :: "r"(value), "I"(port) );
        }

    };

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
    auto cached(volatile T &r)
    {
        return cached_io(r);
    }

    template <class T>
    auto cached(const T &io)
    {
        return cached_io(io.ref());
    }

    template <typename T>
    auto forced(volatile T &r)
    {
        return forced_io(r);
    }

    template <class T>
    auto forced(const T &io)
    {
        return forced_io(io.ref());
    }

    template <typename T>
    auto raw(volatile T &r)
    {
        return raw_io(r);
    }

    template <class T>
    auto raw(const T &io)
    {
        return raw_io(io.ref());
    }

    template <typename T>
    T read(volatile T &r)
    {
        return forced(r).read();
    }

    template <class T>
    T read(const T &io)
    {
        return forced(io.ref()).read();
    }

    template <typename T>
    void write(volatile T &r, T value)
    {
        forced(r).write(value);
    }

    template <class T>
    void write(const T &io, T value)
    {
        forced(io).write(value);
    }

    template <typename T>
    T raw_read(volatile T &r)
    {
        return raw(r).read();
    }

    template <class T>
    T raw_read(const T &io)
    {
        return raw(io).read();
    }

    template <typename T, typename T2>
    void raw_write(volatile T &r, T2 value)
    {
        raw(r).write(value);
    }

    template <class T>
    void raw_write(const T &io, T value)
    {
        raw(io).write(value);
    }

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

        constexpr bitfield(T &a, int o, int l): addr(a), vaddr(a), offset(o), length(l) { }
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

        constexpr ioreg(T &a): addr(a), vaddr(a) { }
        constexpr ioreg(volatile T &a): addr((T &)a), vaddr(a) { }
        constexpr ioreg(const ioreg &) = default;

        uint8_t raw_read() const
        {
            const uint8_t port = reinterpret_cast<int>(&addr) - 0x20;
            // здесь не нужен ни "memory" ни __volatile__
            uint8_t value;
            __asm__  ( "in %0, %1" : "=r"(value) : "I"(port) );
            return value;
        }

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
            if ( value ) vaddr |= (1 << offset);
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
