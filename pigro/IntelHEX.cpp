#include "IntelHEX.h"

#include <fstream>

namespace
{

    /**
     * Конверировать шестнадцатеричную цифру в число
     */
    uint8_t at_hex_digit(char ch)
    {
        if ( ch >= '0' && ch <= '9' ) return ch - '0';
        if ( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
        if ( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
        throw pigro::exception("at_hex_digit(): unexpected character");
    }

    /**
     * Прочитать байт
     */
    uint8_t at_hex_get_byte(const char *line, int i)
    {
        int offset = i * 2 + 1;
        return at_hex_digit(line[offset]) * 0x10 + at_hex_digit(line[offset+1]);
    }

}

namespace pigro
{

    void IntelHEX::open(const std::string &path)
    {
        std::fstream f(path, std::ios_base::in);
        if ( f.fail() ) throw pigro::exception("fail to open file: " + path);

        int lineno = 0;
        while ( !f.eof() )
        {
            line_t hexline;
            std::string line;
            std::getline(f, line);
            lineno++;
            if ( line.length() < 11 ) throw pigro::exception("wrong hex-file: line length < 11");
            if ( line[0] != ':' )
            {
                throw pigro::exception("wrong hex-file: line doen't start from ':'");
            }

            uint8_t len = at_hex_get_byte(line.data(), 0);
            int bytelen = len + 5;
            size_t charlen = bytelen * 2 + 1;
            if ( line.length() < charlen ) throw pigro::exception("wrong hex-file: line too short");

            for(int i = 0; i < bytelen; i++)
            {
                hexline.bytes[i] = at_hex_get_byte(line.data(), i);
            }

            uint8_t sum = 0;
            for(int i = 0; i < (bytelen-1); i++)
            {
                sum -= hexline.bytes[i];
            }

            //printf("line=%d sum=0x%02X expected=0x%02X\n", lineno, sum, hexline.checksum());
            if ( hexline.checksum() != sum ) throw pigro::exception("wrong hex-file: wrong checksum");

            uint8_t type = hexline.type();
            uint16_t addr = hexline.addr();
            //const uint8_t *data = hexline.data();

            printf("line[%2d] len=%3d type=%d, addr=0x%04X charlen=%ld/%ld\n", lineno, len, type, addr, charlen, line.length());

            if ( type == 1 )
            {
                printf("end of hex\n");
                return;
            }
        }

    }

}
