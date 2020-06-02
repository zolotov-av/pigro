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
        rows.clear();

        std::fstream f(path, std::ios_base::in);
        if ( f.fail() ) throw pigro::exception("fail to open file: " + path);

        int lineno = 0;
        while ( !f.eof() )
        {
            row_t row {};
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
                row.bytes[i] = at_hex_get_byte(line.data(), i);
            }

            uint8_t sum = 0;
            for(int i = 0; i < (bytelen-1); i++)
            {
                sum -= row.bytes[i];
            }

            if ( row.checksum() != sum ) throw pigro::exception("wrong hex-file: wrong checksum");

            uint8_t type = row.type();
            uint16_t addr = row.addr();

            //printf("line[%2d] len=%3d type=%d, addr=0x%04X charlen=%ld/%ld\n", lineno, len, type, addr, charlen, line.length());

            rows.push_back(std::move(row));

            if ( type == 1 )
            {
                printf("end of hex\n");
                printf("rows count: %ld\n", rows.size());
                return;
            }
        }

    }

    AVR_Data IntelHEX::split_pages(const AVR_Info &avr)
    {
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
                    if ( addr >= flash_size ) throw pigro::exception("image too big");
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

}
