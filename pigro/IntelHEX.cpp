#include "IntelHEX.h"

#include <nano/TextReader.h>
#include <nano/string.h>

/**
 * Прочитать байт
 */
static uint8_t parse_hex_byte(const char *line, int i)
{
    int offset = i * 2 + 1;
    return nano::parse_hex_digit(line[offset]) * 0x10 + nano::parse_hex_digit(line[offset+1]);
}

void IntelHEX::open(const std::string &path)
{
    rows.clear();

    nano::TextReader f(path);

    while ( !f.eof() )
    {
        row_t row {};
        std::string line = f.readLine();
        if ( line.length() < 11 ) throw nano::exception("wrong hex-file: line length < 11");
        if ( line[0] != ':' )
        {
            throw nano::exception("wrong hex-file: line doen't start from ':'");
        }

        uint8_t len = parse_hex_byte(line.data(), 0);
        int bytelen = len + 5;
        size_t charlen = bytelen * 2 + 1;
        if ( line.length() < charlen ) throw nano::exception("wrong hex-file: line too short");

        for(int i = 0; i < bytelen; i++)
        {
            row.bytes[i] = parse_hex_byte(line.data(), i);
        }

        uint8_t sum = 0;
        for(int i = 0; i < (bytelen-1); i++)
        {
            sum -= row.bytes[i];
        }

        if ( row.checksum() != sum ) throw nano::exception("wrong hex-file: wrong checksum");

        rows.push_back(std::move(row));

        if ( row.type() == 1 )
        {
            return;
        }
    }

    throw nano::exception("wrong hex-file: unexpected end of file");
}
