#ifndef INTELHEX_H
#define INTELHEX_H

#include "serial.h"

#include <string_view>

namespace pigro
{

    /**
     * @brief The Intel HEX file format
     */
    class IntelHEX
    {
    private:



    public:

        struct line_t
        {
            uint8_t bytes[256 + 5];

            uint8_t length() const { return bytes[0]; }
            uint16_t addr() const { return bytes[1] * 0x100 + bytes[2]; }
            uint8_t type() const { return bytes[3]; }
            uint8_t checksum() const { return bytes[length()+4]; }
            const uint8_t* data() const { return &bytes[4]; }
        };

        void open(const std::string &path);

    };

}

#endif // INTELHEX_H
