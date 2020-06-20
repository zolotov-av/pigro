#ifndef ARMXX_BITBAND_H
#define ARMXX_BITBAND_H

#include <cstdint>

namespace arm
{

    // DO NOT IMPLEMENT
    uint32_t* unsupported_bitbang();

    constexpr inline uint32_t bitband_addr(uint32_t addr, unsigned bit)
    {
        if ( bit < 8 )
        {
            if ( (addr >= 0x20000000) && (addr <= 0x200FFFFF) )
            {
                const uint32_t byte_offset = addr - 0x20000000;
                return 0x22000000 + (byte_offset * 32) + (bit * 4);
            }

            if ( (addr >= 0x40000000) && (addr <= 0x400FFFFF) )
            {
                const uint32_t byte_offset = addr - 0x40000000;
                return 0x42000000 + (byte_offset * 32) + (bit * 4);
            }

        }

        return 0;
    }

    constexpr inline volatile uint32_t* bitband_ptr(uint32_t addr, unsigned bit)
    {
        if ( uint32_t a = bitband_addr(addr, bit) )
        {
            return reinterpret_cast<uint32_t*>(a);
        }

        return unsupported_bitbang();
    }

    constexpr inline volatile uint32_t& bitband_ref(uint32_t addr, unsigned bit)
    {
        return *bitband_ptr(addr, bit);
    }

    template <typename T>
    inline bool bitband_read(const volatile T &ref, unsigned bit)
    {
        if ( (bit / 8) > sizeof(T) ) return unsupported_bitbang();
        return bitband_ref(reinterpret_cast<uint32_t>(&ref) + bit / 8, bit % 8) & 1;
    }

    template <typename T>
    inline void bitband_write(volatile T &ref, unsigned bit, bool value)
    {
        if ( (bit / 8) > sizeof(T) ) unsupported_bitbang();
        bitband_ref(reinterpret_cast<uint32_t>(&ref) + bit / 8, bit % 8) = value ? 1 : 0;
    }

}

#endif // ARMXX_BITBAND_H
