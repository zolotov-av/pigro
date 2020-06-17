#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <array>

using DeviceCode = std::array<uint8_t, 3>;

struct DeviceInfo
{
    DeviceCode signature;

    std::string type;

    bool paged;

    uint16_t page_word_size;
    uint16_t page_count;

    uint16_t eeprom_page_size;
    uint16_t eeprom_page_count;

    constexpr static bool is_power_of_two(uint16_t value)
    {
        if ( value == 0 ) return false;
        while ( value > 1 )
        {
            if ( value % 2 == 1 ) return false;
            value = value / 2;
        }
        return true;
    }

    bool valid() const
    {
        if ( page_word_size == 0 || page_count == 0 ) return false;
        if ( !is_power_of_two(page_word_size) ) return false;

        if ( type == "avr ")
        {
            uint32_t size = flash_size();
            return (size > 0) && (size < 0x10000);
        }

        if ( type == "arm" )
        {
            return flash_size() > 0;
        }

        return false;
    }

    uint32_t flash_size() const { return page_byte_size() * page_count; }
    uint32_t eeprom_size() const { return eeprom_page_size * eeprom_page_count; }

    uint16_t page_byte_size() const { return page_word_size * 2; }
    uint16_t word_mask() const { return page_word_size - 1; }
    uint16_t byte_mask() const { return page_byte_size() - 1; }
    uint16_t page_mask() const { return 0xFFFF ^ word_mask(); }
};


#endif // DEVICEINFO_H
