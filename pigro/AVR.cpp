#include "AVR.h"

#include "IntelHEX.h"
#include <nano/IniReader.h>
#include <cstdlib>


/**
 * Первести шестнадцатеричное число из строки в целочисленное значение
 */
uint32_t at_hex_to_int(const char *s)
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

AVR::FirmwareData avr_load_from_hex(const AVR_Info &avr, const std::string &path)
{
    const auto rows = IntelHEX(path).rows;

    const uint16_t page_byte_size = avr.page_byte_size();
    const uint16_t page_mask = avr.page_mask();
    const uint16_t byte_mask = avr.byte_mask();
    const uint32_t flash_size = avr.flash_size();

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

AVR::~AVR()
{

}

void AVR::action_test()
{
    printf("\nAVR::action_test()\n\n");

    throw nano::exception(std::string(__func__) + " not implemented");
}

void AVR::isp_chip_info()
{
    printf("\nAVR::isp_chip_info()\n\n");

    isp_program_enable();

    check_chip_info();

    isp_program_disable();
}

void AVR::isp_check_firmware(const PigroDriver::FirmwareData &pages)
{
    printf("\nAVR::isp_check_firmware()\n");

    isp_program_enable();

    check_chip_info();
    check_fuses();

    for(const auto &[page_addr, page] : pages)
    {
        uint8_t counter = 0;
        const size_t size = page.data.size();
        for(size_t i = 0; i < size; i++)
        {
            const uint16_t addr = (page_addr * 2) + i;
            if ( counter == 0 ) printf("MEM[0x%04X]", addr);
            uint8_t byte = isp_read_memory(addr);
            printf("%s", (page.data[i] == byte ? "." : "*" ));
            if ( counter == 0x1F ) printf("\n");
            counter = (counter + 1) & 0x1F;
        }
    }

    isp_program_disable();
}

void AVR::isp_write_firmware(const PigroDriver::FirmwareData &pages)
{
    printf("\nAVR::isp_write_firmware()\n\n");

    isp_program_enable();

    if ( !check_chip_info() )
    {
        throw nano::exception("isp_write_firmware() reject: wrong chip signature");
    }

    if ( !chip_info().valid() || !chip_info().paged )
    {
        throw nano::exception("isp_write_firmware() reject: unsupported chip");
    }

    chip_erase();

    for(const auto &[page_addr, page] : pages)
    {
        uint8_t counter = 0;
        const size_t size = page.data.size();
        for(size_t i = 0; i < size; i++)
        {
            const uint16_t addr = (page_addr * 2) + i;
            if ( counter == 0 ) printf("MEM[0x%04X]", addr);
            isp_load_memory_page((page_addr * 2) + i, page.data[i]);
            printf(".");
            if ( counter == 0x1F ) printf("\n");
            counter = (counter + 1) & 0x1F;

        }
        isp_write_memory_page(page_addr);
    }

    isp_program_disable();
}

void AVR::isp_read_fuse()
{
    printf("\nAVR::isp_read_fuse()\n\n");

    isp_program_enable();

    check_chip_info();

    uint8_t fuse_lo = isp_read_fuse_low();  // cmd_isp_io(0x50000000) & 0xFF;
    uint8_t fuse_hi = isp_read_fuse_high(); // cmd_isp_io(0x58080000) & 0xFF;
    uint8_t fuse_ex = isp_read_fuse_ext();  // cmd_isp_io(0x50080000) & 0xFF;

    const char *status;

    if ( auto s = get_option("fuse_low"); !s.empty() )
    {
        const uint8_t x = parse_fuse(s, "fuse_low (pigro.ini)");
        status = (x == fuse_lo) ? " ok " : "diff";
    }
    else status = " NA ";
    printf("fuse low:  0x%02X [%s]\n", fuse_lo, status);

    if ( auto s = get_option("fuse_high"); !s.empty() )
    {
        const uint8_t x = parse_fuse(s, "fuse_high (pigro.ini)");
        status = (x == fuse_hi) ? " ok " : "diff";
    }
    else status = " NA ";
    printf("fuse high: 0x%02X [%s]\n", fuse_hi, status);

    if ( auto s = get_option("fuse_ext"); !s.empty() )
    {
        const uint8_t x = parse_fuse(s, "fuse_ext (pigro.ini)");
        status = (x == fuse_ex) ? " ok " : "diff";
    }
    else status = " NA ";
    printf("fuse ext:  0x%02X [%s]\n", fuse_ex, status);

    isp_program_disable();
}

void AVR::isp_write_fuse()
{
    printf("\nAVR::isp_write_fuse()\n\n");

    isp_program_enable();

    if ( !check_chip_info() )
    {
        throw nano::exception("isp_write_firmware() reject: wrong chip signature");
    }


    if ( auto s = get_option("fuse_low"); !s.empty() )
    {
        const uint8_t x = parse_fuse(s, "fuse_low (pigro.ini)");
        isp_write_fuse_low(x);
    }

    if ( auto s = get_option("fuse_high"); !s.empty() )
    {
        const uint8_t x = parse_fuse(s, "fuse_high (pigro.ini)");
        isp_write_fuse_high(x);
    }

    if ( auto s = get_option("fuse_ext"); !s.empty() )
    {
        const uint8_t x = parse_fuse(s, "fuse_ext (pigro.ini)");
        isp_write_fuse_ext(x);
    }

    isp_program_disable();
}

void AVR::isp_chip_erase()
{
    printf("\nAVR::isp_chip_erase()\n\n");

    isp_program_enable();

    chip_erase();

    isp_program_disable();
}
