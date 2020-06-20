#ifndef TINY_IO_H
#define TINY_IO_H

namespace tiny
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

    template <typename T> class cached_io;
    template <typename T> class forced_io;

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
    auto forced(T &r)
    {
        return forced_io(r);
    }

}

#endif // TINY_IO_H
