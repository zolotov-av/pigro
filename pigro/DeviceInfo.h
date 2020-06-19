#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <array>
#include <optional>
#include <nano/config.h>
#include <nano/ini.h>
#include <nano/math.h>

using DeviceCode = std::array<uint8_t, 3>;

struct DeviceInfo
{
    DeviceCode signature;

    std::string type;

    bool paged;

    uint32_t page_word_size;
    uint32_t page_count;

    uint32_t eeprom_page_size;
    uint32_t eeprom_page_count;

    bool valid() const
    {
        if ( page_word_size == 0 || page_count == 0 ) return false;
        if ( !nano::is_power_of_two(page_word_size) ) return false;

        if ( type == "avr")
        {
            uint32_t size = flash_size();
            return (size > 0) && (size < 0x20000);
        }

        if ( type == "arm" )
        {
            return flash_size() > 0;
        }

        return false;
    }

    uint32_t flash_size() const { return page_byte_size() * page_count; }
    uint32_t eeprom_size() const { return eeprom_page_size * eeprom_page_count; }

    uint32_t page_byte_size() const { return page_word_size * 2; }

    /**
     * Загрузить описание чипа из файла
     */
    static std::optional<nano::options> LoadFromFile(const std::string &name, const std::string &path)
    {
        try
        {
            nano::config ini = nano::ini::loadFromFile(path);
            if ( ini.haveSection(name) )
            {
                return ini.section(name);
            }

            return {};
        }
        catch (const nano::exception &e)
        {
            return {};
        }
    }

    /**
     * Найти описание чипа по имени
     */
    static std::optional<nano::options> LoadByName(const std::string &name)
    {
        if ( auto device = LoadFromFile(name, "pigro.ini"); device.has_value() )
        {
            return device;
        }

        const std::string home = getenv("HOME");

        if ( auto device = LoadFromFile(name, home + "/.pigro/devices.ini"); device.has_value() )
        {
            return device;
        }

        if ( auto device = LoadFromFile(name, home + "/.pigro/" + name + ".ini"); device.has_value() )
        {
            return device;
        }

        if ( auto device = LoadFromFile(name, "/usr/share/pigro/devices.ini"); device.has_value() )
        {
            return device;
        }

        if ( auto device = LoadFromFile(name, "/usr/share/pigro/" + name + ".ini"); device.has_value() )
        {
            return device;
        }

        return {};
    }
};


#endif // DEVICEINFO_H
