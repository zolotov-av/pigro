#ifndef NANO_MATH_H
#define NANO_MATH_H

#include <type_traits>

namespace nano
{

    template <typename Int>
    constexpr static bool is_power_of_two(Int value)
    {
        if ( value == 0 ) return false;
        static_assert(std::is_unsigned<Int>::value);
        if ( value < 0 ) return false;
        while ( value > 1 )
        {
            if ( value % 2 == 1 ) return false;
            value = value / 2;
        }
        return true;
    }

}

#endif // NANO_MATH_H
