#include "AVR.h"

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

QString AVR::getIspChipInfo()
{
    printf("\nAVR::isp_chip_info()\n\n");

    isp_program_enable();

    const auto info = isp_read_chip_info();
    const bool status = (info == parseDeviceCode(chip_info().value("device_code")));
    const char *status_str = status ? "[ ok ]" : "[diff]";
    char buf[80];
    sprintf(buf, "0x%02X, 0x%02X, 0x%02X %s", info[0], info[1], info[2], status_str);

    check_chip_info();

    isp_program_disable();

    return QString::fromLatin1(buf);
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

void AVR::isp_stat_firmware(const FirmwareData &pages)
{
    printf("\nAVR::isp_stat_firmware()\n\n");

    check_firmware(pages, true);
}

void AVR::isp_check_firmware(const FirmwareData &pages)
{
    printf("\nAVR::isp_check_firmware()\n");

    isp_program_enable();

    check_chip_info();
    check_fuses();

    uint8_t counter = 0;
    for(const auto &[page_addr, page] : pages)
    {
        const size_t size = page.data.size();
        for(size_t i = 0; i < size; i++)
        {
            const uint32_t addr = page_addr + i;
            if ( counter == 0 ) printf("MEM[0x%04X]", addr);
            uint8_t byte = isp_read_memory(addr);
            printf("%s", (page.data[i] == byte ? "." : "*" ));
            if ( counter == 0x1F ) printf("\n");
            counter = (counter + 1) & 0x1F;
        }
    }

    isp_program_disable();
}

void AVR::isp_write_firmware(const FirmwareData &pages)
{
    printf("\nAVR::isp_write_firmware()\n\n");

    if ( !check_firmware(pages, false) )
    {
        error(" AVR::isp_write_firmware(): bad firmware");
        check_firmware(pages, true);
        return;
    }

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

    uint8_t counter = 0;
    for(const auto &[page_addr, page] : pages)
    {
        const size_t size = page.data.size();
        for(size_t i = 0; i < size; i++)
        {
            const uint32_t addr = page_addr + i;
            if ( counter == 0 ) printf("MEM[0x%04X]", addr);
            isp_load_memory_page(addr, page.data[i]);
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
