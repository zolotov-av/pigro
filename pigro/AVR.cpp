#include "AVR.h"

#include "IntelHEX.h"
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

AVR::FirmwareData avr_load_from_hex(uint32_t page_size, const std::string &path)
{
    if ( page_size > 0x10000 ) throw nano::exception("avr_load_from_hex(): page_size too large");
    if ( !DeviceInfo::is_power_of_two(page_size) ) throw nano::exception("avr_load_from_hex(): page_size is not power of two");

    const auto rows = IntelHEX(path).rows;

    /*
    uint32_t flash_size() const { return page_byte_size() * page_count; }
    uint32_t eeprom_size() const { return eeprom_page_size * eeprom_page_count; }

    uint16_t page_byte_size() const { return page_word_size * 2; }
    uint16_t word_mask() const { return page_word_size - 1; }
    uint16_t byte_mask() const { return page_byte_size() - 1; }
    uint16_t page_mask() const { return 0xFFFF ^ word_mask(); }
    */

    const uint16_t page_byte_size = page_size;
    const uint16_t page_word_size = page_size / 2;
    const uint16_t word_mask = page_word_size - 1;
    const uint16_t page_mask = 0xFFFF ^ word_mask;
    const uint16_t byte_mask = page_byte_size - 1;
    //const uint32_t flash_size = page_byte_size * page_count;

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
                //if ( addr >= flash_size ) throw nano::exception("image too big");
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

uint32_t AVR::page_size() const
{
    return avr.page_byte_size();
}

uint32_t AVR::page_count() const
{
    return avr.page_count;
}

void AVR::action_test()
{
    printf("\nAVR::action_test()\n\n");

    throw nano::exception(std::string(__func__) + " not implemented");
}

void AVR::parse_device_info(const nano::options &options)
{
    printf("AVR::parse_device_info()\n");
    avr.type = options.value("type", "avr");
    avr.signature = parseDeviceCode(options.value("device_code"));
    avr.page_word_size = atoi(options.value("page_size").c_str());
    avr.page_count = atoi(options.value("page_count").c_str());
    avr.paged = nano::parse_bool(options.value("paged", "yes"));

    if ( verbose() )
    {
        if ( avr.paged )
        {
            std::cout << "page_size: " << int(avr.page_word_size) << " words\n";
            std::cout << "page_count: " << int(avr.page_count) << "\n";
        }
        std::cout << "flash_size: " << ((avr.flash_size()+1023) / 1024) << "k\n";
    }

    if ( !avr.paged )
    {
        warn("unsupported chip, only Write Page supported");
    }
    else if ( !avr.valid() )
    {
        warn("invalid or unsupported chip data, check database");
    }
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

    if ( !avr.valid() || !avr.paged )
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
