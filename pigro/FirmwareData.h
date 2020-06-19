#ifndef FIRMWAREDATA_H
#define FIRMWAREDATA_H

#include <cstdint>
#include <map>
#include <vector>
#include <string>

#include <nano/math.h>

#include "IntelHEX.h"


struct PageData
{
    uint32_t addr;
    std::vector<uint8_t> data;

    uint32_t page_size() const { return data.size(); }
};

class FirmwareData: public std::map<uint32_t, PageData>
{
public:

    static FirmwareData LoadFromHex(const IntelHEX &hex, const uint32_t page_size = 1024, const uint8_t page_fill = 0xFF)
    {
        if ( !nano::is_power_of_two(page_size) ) throw nano::exception("FirmwareData: page_size is not poewr of two: " + std::to_string(page_size));

        const uint32_t byte_mask = page_size - 1;
        const uint32_t page_mask = ~byte_mask;

        FirmwareData pages;
        uint32_t LoadAddress = 0;
        for(const auto &row : hex.rows)
        {
            if ( row.type() == 0x04 )
            {
                if ( row.length() != 2 ) throw nano::exception("FirmwareData: wrong LBA record (IntelHEX)");
                LoadAddress = (row.data()[0] << 24) | (row.data()[1] << 16);
                continue;
            }

            if ( row.type() == 0x02 )
            {
                throw nano::exception("FirmwareData: unsupported Extended Segment Address Record (IntelHEX)");
            }

            if ( row.type() == 0 )
            {
                const uint8_t *row_data = row.data();
                const uint32_t row_addr = LoadAddress + row.addr();
                const uint32_t row_size = row.length();

                for(uint32_t i = 0; i < row_size; i++)
                {
                    const uint32_t byte_addr = row_addr + i;
                    const uint32_t page_addr = byte_addr & page_mask;
                    const uint32_t byte_offset = byte_addr & byte_mask;
                    PageData &page = pages[page_addr];
                    if ( page.data.size() == 0 )
                    {
                        page.addr = page_addr;
                        page.data.resize(page_size);
                        std::fill(page.data.begin(), page.data.end(), page_fill);
                    }
                    page.data[byte_offset] = row_data[i];
                }
            }
        }

        return pages;

    }

    static FirmwareData LoadFromFile(const std::string &path, uint32_t page_size = 1024, const uint8_t page_fill = 0xFF)
    {
        if ( !nano::is_power_of_two(page_size) ) throw nano::exception("FirmwareData: page_size is not power of two");
        return LoadFromHex(IntelHEX(path), page_size, page_fill);
    }

};

#endif // FIRMWAREDATA_H
