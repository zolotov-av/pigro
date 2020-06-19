#ifndef NANO_STRING_H
#define NANO_STRING_H

#include <string_view>
#include <nano/exception.h>

namespace nano
{

    using string = std::string;

    std::string_view trim(std::string_view text);

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

}

#endif // NANO_STRING_H
