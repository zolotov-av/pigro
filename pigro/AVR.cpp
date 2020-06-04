#include "AVR.h"

#include "IntelHEX.h"
#include <nano/IniReader.h>
#include <cstdlib>

/**
 * Конверировать шестнадцатеричную цифру в число
 */
static uint8_t at_hex_digit(char ch)
{
    if ( ch >= '0' && ch <= '9' ) return ch - '0';
    if ( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
    if ( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
    throw nano::exception("wrong hex digit");
}

/**
 * Первести шестнадцатеричное число из строки в целочисленное значение
 */
static uint32_t at_hex_to_int(const char *s)
{
    if ( s[0] == '0' && (s[1] == 'x' || s[1] == 'X') ) s += 2;

    uint32_t r = 0;

    while ( *s )
    {
        char ch = *s++;
        uint8_t hex = at_hex_digit(ch);
        if ( hex > 0xF ) throw nano::exception("wrong hex digit");
        r = r * 0x10 + hex;
    }
    return r;
}

AVR::DeviceCode AVR::parseDeviceCode(const std::string &code)
{
    if ( code.empty() ) throw nano::exception("wrong device code: " + code);
    const uint32_t hex = at_hex_to_int(code.c_str());
    const uint8_t b000 = hex >> 16;
    const uint8_t b001 = hex >> 8;
    const uint8_t b002 = hex;
    return {b000, b001, b002};
}

AVR::FirmwareData avr_load_from_hex(const AVR_Info &avr, const std::string &path)
{
    const auto rows = IntelHEX(path).rows;

    const uint16_t page_byte_size = avr.page_byte_size();
    const uint16_t page_mask = avr.page_mask();
    const uint16_t byte_mask = avr.byte_mask();
    const uint16_t flash_size = avr.flash_size();

    AVR_Data pages;
    for(const auto &row : rows)
    {
        if ( row.type() == 0 )
        {
            const uint16_t row_addr = row.addr();
            const uint8_t *row_data = row.data();
            for(uint16_t i = 0; i < row.length(); i++)
            {
                const uint16_t addr = row_addr + i;
                if ( addr >= flash_size ) throw nano::exception("image too big");
                const uint16_t page_addr = (addr / 2) & page_mask;
                const uint16_t offset = addr & byte_mask;
                AVR_Page &page = pages[page_addr];
                if ( page.data.size() == 0 )
                {
                    page.addr = page_addr;
                    page.data.resize(page_byte_size);
                    std::fill(page.data.begin(), page.data.end(), 0xFF);

                }
                page.data[offset] = row_data[i];
            }
        }
    }

    return pages;
}

static std::optional<AVR::DeviceInfo> findInFile(const std::string &name, const std::string &path)
{
    try
    {
        nano::IniReader ini(path);
        if ( ini.haveSection(name) )
        {
            AVR::DeviceInfo avr;
            avr.signature = AVR::parseDeviceCode(ini.value(name, "device_code"));
            avr.page_word_size = atoi(ini.value(name, "page_size").c_str());
            avr.page_count = atoi(ini.value(name, "page_count").c_str());
            return avr;
        }

        return {};
    }
    catch (const nano::exception &e)
    {
        return {};
    }
}

std::optional<AVR::DeviceInfo> AVR::findDeviceByName(const std::string &name)
{
    if ( auto device = findInFile(name, "./pigro.ini"); device.has_value() )
    {
        return device;
    }

    const std::string home = getenv("HOME");

    if ( auto device = findInFile(name, home + "/.pigro/devices.ini"); device.has_value() )
    {
        return device;
    }

    if ( auto device = findInFile(name, home + "~/.pigro/" + name + ".ini"); device.has_value() )
    {
        return device;
    }

    if ( auto device = findInFile(name, "/usr/share/pigro/devices.ini"); device.has_value() )
    {
        return device;
    }

    if ( auto device = findInFile(name, "/usr/share/pigro/" + name + ".ini"); device.has_value() )
    {
        return device;
    }

    return {};
}
