#ifndef INTELHEX_H
#define INTELHEX_H

#include <nano/serial.h>

#include <map>
#include <list>
#include <array>
#include <vector>
#include <string_view>

namespace pigro
{

    using AVR_Signature = std::array<uint8_t, 3>;

    struct AVR_Info
    {
        AVR_Signature signature;
        uint8_t page_word_size;
        uint8_t page_count;

        uint8_t eeprom_page_size;
        uint8_t eeprom_page_count;

        uint32_t flash_size() const { return page_byte_size() * page_count; }
        uint32_t eeprom_size() const { return eeprom_page_size * eeprom_page_count; }

        uint16_t page_byte_size() const { return page_word_size * 2; }
        uint16_t word_mask() const { return page_word_size - 1; }
        uint16_t byte_mask() const { return page_byte_size() - 1; }
        uint16_t page_mask() const { return 0xFFFFF ^ word_mask(); }

    };

    struct AVR_Page
    {
        uint16_t addr;
        std::vector<uint8_t> data;

        uint16_t page_size() const { return data.size() / 2; }
    };

    using AVR_Data = std::map<uint16_t, AVR_Page>;

    /**
     * @brief The Intel HEX file format
     */
    class IntelHEX
    {
    private:



    public:

        struct row_t
        {
            uint8_t bytes[256 + 5];

            uint8_t length() const { return bytes[0]; }
            uint16_t addr() const { return bytes[1] * 0x100 + bytes[2]; }
            uint8_t type() const { return bytes[3]; }
            uint8_t checksum() const { return bytes[length()+4]; }
            const uint8_t* data() const { return &bytes[4]; }
        };

        std::list<row_t> rows;

        void open(const std::string &path);

        AVR_Data split_pages(const AVR_Info &avr);

    };

}

#endif // INTELHEX_H
