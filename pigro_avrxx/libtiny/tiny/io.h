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

    template <typename T>
    inline T cached_read(const volatile T &var)
    {
        return *reinterpret_cast<const T *>(&var);
    }

    template <typename T>
    inline T forced_read(const volatile T &var)
    {
        return var;
    }

    template <typename T, typename V>
    inline void cached_write(volatile T &var, V value)
    {
        *reinterpret_cast<T*>(&var) = value;
    }

    template <typename T, typename V>
    inline void forced_write(volatile T &var, V value)
    {
        var = value;
    }

    template <typename T> class cached_io;
    template <typename T> class forced_io;

    template <typename T>
    class cached_io
    {
    private:

        volatile T &var;

    public:

        T &ref() const { return *reinterpret_cast<T *>(&var); }

        constexpr cached_io() = delete;
        constexpr cached_io(volatile T &v): var(v) { }
        constexpr cached_io(const cached_io &) = default;
        constexpr cached_io(cached_io &&) = delete;
        template <class IO> constexpr cached_io(const IO &io): var(io.ref) { }

        T read() const
        {
            return tiny::cached_read(var);
        }

        template <typename V>
        void write(V value) const
        {
            tiny::cached_write(var, value);
        }

    };

    template <typename T>
    class forced_io
    {
    private:

        volatile T &var;

    public:

        volatile T &ref() const { return var; }

        constexpr forced_io() = delete;
        constexpr forced_io(volatile T &v): var(v) { }
        constexpr forced_io(const forced_io &) = default;
        constexpr forced_io(forced_io &&) = delete;
        template <class IO> constexpr forced_io(const IO &io): var(io.ref) { }

        T read() const
        {
            return tiny::forced_read(var);
        }

        template <typename V>
        void write(V value) const
        {
            tiny::forced_write(var, value);
        }

    };

    template <typename T>
    constexpr auto cached(volatile T &var)
    {
        return cached_io(var);
    }

    template <typename T>
    constexpr auto forced(volatile T &var)
    {
        return forced_io(var);
    }

    template <class T>
    constexpr auto cached(const T &io)
    {
        return cached_io(io.ref());
    }

    template <class T>
    constexpr auto forced(const T &io)
    {
        return forced_io(io.ref());
    }

}

#endif // TINY_IO_H
