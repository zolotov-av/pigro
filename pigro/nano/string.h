#ifndef NANO_STRING_H
#define NANO_STRING_H

#include <string_view>
#include <nano/exception.h>

namespace nano
{

    using string = std::string;

    std::string_view trim(std::string_view text);

    inline uint8_t parse_decimal_digit(char ch)
    {
        if ( ch >= '0' && ch <= '9' ) return ch - '0';
        throw nano::exception("wrong decimal digit");
    }

    /**
     * Конверировать шестнадцатеричную цифру в число
     */
    inline uint8_t parse_hex_digit(char ch)
    {
        if ( ch >= '0' && ch <= '9' ) return ch - '0';
        if ( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
        if ( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
        throw nano::exception("wrong hex digit");
    }

    /**
     * Первести шестнадцатеричное число из строки в целочисленное значение
     */
    template <typename Int>
    Int parse_hex(const char *s)
    {
        if ( s[0] == '0' && (s[1] == 'x' || s[1] == 'X') ) s += 2;

        Int r = 0;

        while ( *s )
        {
            char ch = *s++;
            uint8_t hex = nano::parse_hex_digit(ch);
            r = r * 0x10 + hex;
        }
        return r;
    }

    template <typename Int>
    Int parse_hex(const std::string &s)
    {
        return parse_hex<Int>(s.c_str());
    }

    inline int parse_int(const std::string &s)
    {
        return atoi(s.c_str());
    }

    inline uint32_t parse_size(const char *str)
    {
        const char *s = str;
        if ( s[0] == '0' && ((s[1] == 'x') || (s[1] == 'X')) ) return nano::parse_hex<uint32_t>(s);

        uint32_t value = 0;
        while ( (*s >= '0') && (*s <= '9') )
        {
            value = value * 10 + (*s - '0');
            s++;
        }

        if ( *s == 0 ) return value;

        if ( ((*s == 'k') || (*s == 'K')) && (s[1] == 0) )
        {
            return value * 1024;
        }

        if ( ((*s == 'm') || (*s == 'M')) && (s[1] == 0) )
        {
            return value * (1024*1024);
        }

        throw nano::exception("wrong size suffix: " + std::string(str));
    }

    inline uint32_t parse_size(const std::string &s)
    {
        return parse_size(s.c_str());
    }

}

#endif // NANO_STRING_H
