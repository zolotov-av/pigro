#include "AVR.h"

#include "IntelHEX.h"

AVR_Data avr_load_from_hex(const AVR_Info &avr, const char *path)
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
