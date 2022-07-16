#include "ARM.h"

#include <nano/exception.h>

static void not_implemented_yet(const std::string &func)
{
    throw nano::exception(func + " not implemented yet...");
}

ARM::ARM(PigroLink *link, Pigro *owner): PigroDriver(link, owner)
{

}

ARM::~ARM()
{

}

uint32_t ARM::page_size() const
{
    return arm.page_size;
}

uint32_t ARM::page_count() const
{
    return arm.page_count;
}

QString ARM::getIspChipInfo()
{
    return QStringLiteral("ARM::getIspChipInfo() not implemented yet");
}

FirmwareData ARM::readFirmware()
{
    throw nano::exception("ARM::readFirmware() not implemented yet");
}

void ARM::action_test()
{
    printf("\ntest STM32/JTAG\n");

    check_idcode_raw();
    check_bypass<9, 32>(0x12345678);

    debug_enable();

    printf("flash size: %dkB\n", (read_flash_size()+1023)/1024);


    const uint32_t addr = 0x08000000;
    printf("\n");
    for(int i = 0; i < 8; i++)
    {
        dump_mem32(addr + 4 * i);
    }
    printf("\n");

    dump_mem32(0xE0042000);
    dump_mem32(0x40010800);

    debug_disable();
}

void ARM::parse_device_info(const nano::options &options)
{
    arm.page_size = parse_page_size(options.value("page_size", "1024"));
    arm.flash_size = parse_flash_size(options.value("flash_size"), arm.page_size);
    arm.page_count = arm.flash_size / arm.page_size;
}

void ARM::isp_chip_info()
{
    printf("\nARM::isp_chip_info()\n\n");

    debug_enable();

    warn("ARM::isp_chip_info() not implemented yet");

    debug_disable();
}

void ARM::isp_stat_firmware(const FirmwareData &pages)
{
    printf("\nARM::isp_stat_firmware()\n\n");

    show_info();
    check_firmware(pages, true);
}

void ARM::isp_check_firmware(const FirmwareData &pages)
{
    const auto dataSize = pages.getDataSize();
    beginProgress(0, pages.getDataSize());
    reportMessage("ARM::isp_check_firmware()");

    bool differs = false;

    debug_enable();

    /*
    auto signature = isp_chip_info();
    if ( signature != avr.signature )
    {
        warn("isp_check_firmware(): wrong chip signature");
    }
    */

    char line[128];
    int line_offset = 0;
    uint32_t pos = 0;
    uint8_t counter = 0;
    for(const auto &[page_addr, page] : pages)
    {
        set_memaddr(page_addr);
        const size_t size = page.data.size() / 4;
        for(size_t i = 0; i < size; i++)
        {
            if ( m_cancel )
            {
                debug_disable();
                endProgress();
                throw nano::exception("canceled");
            }

            reportProgress(pos+=4);
            const uint16_t offset = i * 4;
            const uint32_t addr = page_addr + offset;
            if ( counter == 0 )
            {
                line_offset = sprintf(line, "MEM[0x%08X]", addr);
            }

            const uint32_t hex_value = page.data[offset] | (page.data[offset+1] << 8) | (page.data[offset+2] << 16) | (page.data[offset+3] << 24);
            const uint32_t device_value = read_next32();
            if ( device_value != hex_value ) differs = true;

            line[line_offset++] = (device_value == hex_value ? '.' : '*' );
            if ( counter == 0x1F )
            {
                reportMessage(QString::fromUtf8(line, line_offset));
            }
            counter = (counter + 1) & 0x1F;
        }
    }

    if ( counter != 0 )
    {
        reportMessage(QString::fromUtf8(line, line_offset));
    }

    reportMessage(QStringLiteral("pos=%1 dataSize=%2").arg(pos).arg(dataSize));

    if ( differs )
    {
        reportMessage("[ FAIL ] firmware is different");
    }
    else
    {
        reportMessage("[ OK ] firmware is same");
    }

    debug_disable();
    endProgress();
}

void ARM::isp_write_firmware(const FirmwareData &pages)
{
    const auto dataSize = pages.getDataSize();
    beginProgress(0, dataSize);
    reportMessage("ARM::isp_write_firmware()");

    if ( !check_firmware(pages, false) )
    {
        error("ARM::isp_write_firmware(): bad firmware");
        check_firmware(pages, true);
        return;
    }

    debug_enable();

    unlock_fpec();

    reportMessage("TODO check chip info...");
    /* TODO:
    auto signature = isp_chip_info();
    if ( signature != avr.signature )
    {
        throw nano::exception("isp_write_firmware() reject: wrong chip signature");
    }
    if ( !avr.valid() || !avr.paged )
    {
        throw nano::exception("isp_write_firmware() reject: unsupported chip");
    }
    */

    fpec_mass_erase();

    write_fpec(0x10, 1); // FLASH_CR_PG

    printf("flash_cr: 0x%08X\n", read_fpec(0x10));

    char line[128];
    int line_offset = 0;
    uint32_t pos = 0;
    uint8_t counter = 0;
    for(const auto &[page_addr, page] : pages)
    {
        set_memaddr(page_addr);
        const size_t size = page.data.size() / 4;
        for(size_t i = 0; i < size; i++)
        {
            if ( m_cancel )
            {
                write_fpec(0x10, 0); // FLASH_CR_PG
                lock_fpec();

                debug_disable();

                endProgress();
                throw nano::exception("canceled");
            }

            reportProgress(pos+=4);
            const uint32_t offset = i * 4;
            const uint32_t addr = page_addr + offset;
            if ( counter == 0 )
            {
                line_offset = sprintf(line, "MEM[0x%08X]", addr);
            }
            const uint32_t word = page.data[offset] | (page.data[offset+1] << 8) | (page.data[offset+2] << 16) | (page.data[offset+3] << 24);
            cmd_program_next(word);
            line[line_offset++] = '.';
            if ( counter == 0x1F )
            {
                reportMessage(QString::fromUtf8(line, line_offset));
            }
            counter = (counter + 1) & 0x1F;
        }
    }

    if ( counter != 0 )
    {
        reportMessage(QString::fromUtf8(line, line_offset));
    }

    write_fpec(0x10, 0); // FLASH_CR_PG
    lock_fpec();

    debug_disable();

    endProgress();
}

void ARM::isp_chip_erase()
{
    beginProgress(0, 1);
    reportMessage("ARM::isp_chip_erase()");

    debug_enable();

    unlock_fpec();

    reportMessage("TODO check chip info...");
    /* TODO:
    auto signature = isp_chip_info();
    if ( signature != avr.signature )
    {
        throw nano::exception("isp_write_firmware() reject: wrong chip signature");
    }
    if ( !avr.valid() || !avr.paged )
    {
        throw nano::exception("isp_write_firmware() reject: unsupported chip");
    }
    */

    fpec_mass_erase();
    reportProgress(1);

    lock_fpec();

    debug_disable();
    endProgress();
}

void ARM::isp_read_fuse()
{
    reportMessage("ARM::isp_read_fuse()");

    not_implemented_yet(__func__);
}

void ARM::isp_write_fuse()
{
    reportMessage("ARM::isp_write_fuse()");

    not_implemented_yet(__func__);
}
