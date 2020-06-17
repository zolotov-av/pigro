#include "ARM.h"

#include <nano/exception.h>

ARM::~ARM()
{

}

void ARM::isp_check_firmware(const PigroDriver::FirmwareData &pages)
{
    printf("ARM::isp_check_firmware(...)\n\n");

    cmd_jtag_reset();
    arm_check_idcode();
    arm_debug_enable();
    arm_find_memap();

    /*
    auto signature = isp_chip_info();
    if ( signature != avr.signature )
    {
        warn("isp_check_firmware(): wrong chip signature");
    }
    */

    uint8_t counter = 0;
    uint32_t memaddr = 0x08000000;
    arm_set_memaddr(memaddr);
    for(const auto &[page_addr, page] : pages)
    {
        const size_t size = page.data.size() / 4;
        for(size_t i = 0; i < size; i++)
        {
            const uint16_t offset = i * 4;
            const uint32_t addr = 0x08000000 + page_addr * 2 + offset;
            if ( counter == 0 ) printf("MEM[0x%08X]", addr);
            if ( addr != memaddr )
            {
                arm_set_memaddr(addr);
                memaddr = addr;
            }
            uint32_t hex_value = page.data[offset] | (page.data[offset+1] << 8) | (page.data[offset+2] << 16) | (page.data[offset+3] << 24);
            uint32_t device_value = arm_read_mem32();
            memaddr += 4;
            printf("%s", (device_value == hex_value ? "." : "*" ));
            if ( counter == 0x1F ) printf("\n");
            counter = (counter + 1) & 0x1F;
        }
    }

    if ( counter != 0 ) printf("\n");
}

void ARM::isp_write_firmware(const FirmwareData &pages)
{
    printf("\nARM::isp_write_firmware(...)\n\n");
    cmd_jtag_reset();
    arm_check_idcode();
    arm_debug_enable();
    arm_find_memap();

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
    uint32_t memaddr = 0x08000000;
    arm_set_memaddr(memaddr);
    for(const auto &[page_addr, page] : pages)
    {
        const size_t size = page.data.size() / 4;
        for(size_t i = 0; i < size; i++)
        {
            const uint32_t offset = i * 4;
            const uint32_t addr = 0x08000000 + page_addr * 2 + offset;
            if ( counter == 0 ) printf("MEM[0x%08X]", addr);
            if ( addr != memaddr )
            {
                arm_set_memaddr(addr);
                memaddr = addr;
            }
            try
            {
                uint32_t word = page.data[offset] | (page.data[offset+1] << 8) | (page.data[offset+2] << 16) | (page.data[offset+3] << 24);
                arm_fpec_program(word);
                memaddr += 4;
                printf(".");
            }
            catch (const nano::exception &e)
            {
                printf("*\n%s\n", e.message().c_str());
                throw;
            }
            if ( counter == 0x1F )
            {
                printf("\n");
            }
            counter = (counter + 1) & 0x1F;
        }
    }

    if ( counter != 0 ) printf("\n");

    arm_fpec_write_reg(0x10, 0); // FLASH_CR_PG
}

void ARM::isp_chip_erase()
{

}

void ARM::action_test()
{
    printf("\ntest STM32/JTAG\n");

    cmd_jtag_reset();
    arm_check_idcode();
    cmd_jtag_reset();
    arm_debug_enable();
    arm_find_memap();

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
}
