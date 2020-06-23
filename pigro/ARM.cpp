#include "ARM.h"

#include <nano/exception.h>

static void not_implemented_yet(const std::string &func)
{
    throw nano::exception(func + " not implemented yet...");
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

void ARM::action_test()
{
    printf("\ntest STM32/JTAG\n");

    arm_idcode_raw();
    check_bypass<9, 32>(0x12345678);

    arm_debug_enable();

    printf("flash size: %dkB\n", (arm_flash_size()+1023)/1024);

    //arm_fpec_unlock();

    /*
    arm_fpec_mass_erase();
    arm_fpec_program(0x08000000, 0x0102);
    arm_fpec_program(0x08000002, 0x0304);
    arm_fpec_program(0x08000004, 0x0506);
    arm_fpec_program(0x08000006, 0x0708);
    arm_fpec_program(0x08000008, 0x090A);
    arm_fpec_program(0x0800000A, 0x0B0C);
    */

    uint32_t addr = 0x08000000;
    arm_set_memaddr(addr);
    for(int i = 0; i < 4; i++)
    {
        uint32_t value = arm_read_mem32();
        addr += 4;
        printf("MEM[0x%08X]: 0x%08X\n", addr, value);
    }

    arm_set_memaddr(0xE0042000);
    uint32_t value = arm_read_mem32();
    printf("MEM[0xE0042000]: 0x%08X\n", value);
    arm_dump_mem32(0x40010800);

    //arm_clear_sticky(arm_status());


    //arm_check_bypass<32>(0x01020304);
    //arm_check_bypass<35>(0x101010101);
    //arm_check_bypass<38>(0x7FFFFFFFF);

    arm_debug_disable();
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

    arm_debug_enable();

    warn("ARM::isp_chip_info() not implemented yet");

    arm_debug_disable();
}

void ARM::isp_stat_firmware(const FirmwareData &pages)
{
    printf("\nARM::isp_stat_firmware()\n\n");

    show_info();
    check_firmware(pages, true);
}

void ARM::isp_check_firmware(const FirmwareData &pages)
{
    printf("\nARM::isp_check_firmware()\n\n");

    arm_debug_enable();

    /*
    auto signature = isp_chip_info();
    if ( signature != avr.signature )
    {
        warn("isp_check_firmware(): wrong chip signature");
    }
    */

    uint8_t counter = 0;
    for(const auto &[page_addr, page] : pages)
    {
        arm_set_memaddr(page_addr);
        const size_t size = page.data.size() / 4;
        for(size_t i = 0; i < size; i++)
        {
            const uint16_t offset = i * 4;
            const uint32_t addr = page_addr + offset;
            if ( counter == 0 ) printf("MEM[0x%08X]", addr);
            const uint32_t hex_value = page.data[offset] | (page.data[offset+1] << 8) | (page.data[offset+2] << 16) | (page.data[offset+3] << 24);
            const uint32_t device_value = arm_read_mem32();
            printf("%s", (device_value == hex_value ? "." : "*" ));
            if ( counter == 0x1F ) printf("\n");
            counter = (counter + 1) & 0x1F;
        }
    }

    if ( counter != 0 ) printf("\n");

    arm_debug_disable();
}

void ARM::isp_write_firmware(const FirmwareData &pages)
{
    printf("\nARM::isp_write_firmware()\n\n");

    if ( !check_firmware(pages, false) )
    {
        error("ARM::isp_write_firmware(): bad firmware");
        check_firmware(pages, true);
        return;
    }

    arm_debug_enable();

    arm_fpec_unlock();

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

    arm_fpec_mass_erase();

    arm_fpec_write_reg(0x10, 1); // FLASH_CR_PG

    printf("flash_cr: 0x%08X\n", arm_fpec_read_reg(0x10));

    uint8_t counter = 0;
    for(const auto &[page_addr, page] : pages)
    {
        arm_set_memaddr(page_addr);
        const size_t size = page.data.size() / 4;
        for(size_t i = 0; i < size; i++)
        {
            const uint32_t offset = i * 4;
            const uint32_t addr = page_addr + offset;
            if ( counter == 0 ) printf("MEM[0x%08X]", addr);
            const uint32_t word = page.data[offset] | (page.data[offset+1] << 8) | (page.data[offset+2] << 16) | (page.data[offset+3] << 24);
            arm_fpec_program(word);
            printf(".");
            if ( counter == 0x1F )
            {
                printf("\n");
            }
            counter = (counter + 1) & 0x1F;
        }
    }

    if ( counter != 0 ) printf("\n");

    arm_fpec_write_reg(0x10, 0); // FLASH_CR_PG
    arm_fpec_lock();

    arm_debug_disable();
}

void ARM::isp_chip_erase()
{
    printf("\nARM::isp_chip_erase()\n\n");

    arm_debug_enable();

    arm_fpec_unlock();

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

    arm_fpec_mass_erase();

    arm_fpec_lock();

    arm_debug_disable();
}

void ARM::isp_read_fuse()
{
    printf("\nARM::isp_read_fuse()\n\n");

    not_implemented_yet(__func__);
}

void ARM::isp_write_fuse()
{
    printf("\nARM::isp_write_fuse()\n\n");

    not_implemented_yet(__func__);
}
