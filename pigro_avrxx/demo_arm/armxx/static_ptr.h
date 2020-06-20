#ifndef ARMXX_STATIC_PTR_H
#define ARMXX_STATIC_PTR_H

#include <cstdint>
#include <type_traits>

namespace arm
{

    template <typename T>
    class static_ptr
    {
    private:

        uint32_t addr;

    public:

        constexpr static_ptr() = default;
        constexpr static_ptr(const static_ptr &) = default;
        constexpr static_ptr(static_ptr &&) = default;
        static_ptr(volatile T *ptr): addr(reinterpret_cast<uint32_t>(ptr)) { }
        static_ptr(T *ptr): addr(reinterpret_cast<uint32_t>(ptr)) { }

        constexpr uint32_t value() const { return addr; }

        constexpr T* operator -> () const { return reinterpret_cast<T*>(addr); }
        constexpr T& operator * () const { return *reinterpret_cast<T*>(addr); }

        constexpr static_ptr& operator = (const static_ptr &) = default;
        static_ptr& operator = (static_ptr &&) = default;
    };

    static_assert (std::is_trivially_copyable<static_ptr<uint32_t>>::value);
    static_assert (std::is_trivially_move_constructible<static_ptr<uint32_t>>::value);

}

#endif // ARMXX_STATIC_PTR_H
