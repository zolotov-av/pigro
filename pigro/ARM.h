#ifndef ARM_H
#define ARM_H

#include <cstdint>

#include <nano/string.h>
#include <nano/exception.h>
#include "PigroLink.h"
#include "PigroDriver.h"

class ARM: public PigroDriver
{
private:

    uint8_t arm_memap;
    static constexpr uint32_t arm_csw = 0x22000000;

public:

    static constexpr uint8_t IR_BYPASS = 0b1111;
    static constexpr uint8_t IR_IDCODE = 0b1110;
    static constexpr uint8_t IR_DPACC = 0b1010;
    static constexpr uint8_t IR_APACC = 0b1011;
    static constexpr uint8_t IR_ABORT = 0b1000;

    static constexpr uint8_t ACK_OKFAULT = 0b010;
    static constexpr uint8_t ACK_WAIT = 0b001;

    static constexpr uint32_t CORTEX_M3_IDCODE = 0x3BA00477;

    static constexpr uint32_t CSYSPWRUPACK = 1 << 31; // RO  System power-up acknowledge.
    static constexpr uint32_t CSYSPWRUPREQ = 1 << 30; // R/W System power-up request.
    static constexpr uint32_t CDBGPWRUPACK = 1 << 29; // Debug power-up acknowledge.
    static constexpr uint32_t CDBGPWRUPREQ = 1 << 28; // R/W Debug power-up request.

    static constexpr bool xpacc_read = false;
    static constexpr bool xpacc_write = true;

    struct DeviceInfo
    {
        uint32_t page_size;
        uint32_t page_count;
        uint32_t flash_size;
    };

    DeviceInfo arm;

    ARM(PigroLink *link): PigroDriver(link) { }
    ~ARM() override;

    void cmd_jtag_reset()
    {
        //printf("cmd_jtag_reset()\n");
        packet_t pkt;
        pkt.cmd = 5;
        pkt.len = 1;
        pkt.data[0] = 0;
        auto status = send_packet(pkt);
        if ( !status ) throw nano::exception("cmd_jtag_reset() fail");
    }

    template <uint8_t bitcount>
    uint32_t arm_io(uint8_t ir, uint64_t value)
    {
        constexpr uint8_t bytecount = (bitcount + 7) / 8;
        if ( bytecount + 2 > PACKET_MAXLEN ) throw nano::exception("arm_io() packet to long: " + std::to_string(bitcount));
        packet_t pkt;
        pkt.cmd = 8;
        pkt.len = bytecount + 2;
        pkt.data[0] = ir;
        pkt.data[1] = bitcount;
        for(int i = 0; i < bytecount; i++)
        {
            pkt.data[i+2] = value & 0xFF;
            value = value >> 8;
        }

        //dump_packet("arm_io() send", pkt);
        send_packet(pkt);
        recv_packet(pkt);
        //dump_packet("arm_io() recv", pkt);

        if ( pkt.cmd != 8 ) throw nano::exception("arm_io() wrong reply: cmd=" + std::to_string(pkt.cmd));
        if ( pkt.len != bytecount + 2 ) throw nano::exception("arm_io() wrong len: " + std::to_string(pkt.len));

        uint64_t result = 0;
        uint64_t mul = 1;
        for(int i = 0; i < bytecount; i++)
        {
            result = result | (pkt.data[i+2] * mul);
            mul = mul * 256;
        }
        return result;
    }

    /**
     * Error code
     *
     * First hex-digit is error code itself.
     * Second hex-digit is ACK-code
     *
     * ACK-codes (see ARM Debug Interface Architecture Specification):
     *   0x01 (ACK_WAIT)
     *   0x02 (ACK_OKFAULT)
     *   ... other is a fault (reserved codes)
     *
     * 0x00 - success
     * 0x1X - I/O failure (unexpected reply from shift-ir)
     * 0x2X - command failure (CTRL/STAT reports some sticky flags)
     * 0x4X - select failure (write to SELECT register)
     */
    using error_t = uint8_t;

    void check_error(std::string func, error_t error)
    {
        if ( error == 0 ) return /* ok */;
        if ( error & 0x40 ) throw nano::exception(func + " select failure (write to SELECT register)");
        if ( error & 0x20 ) throw nano::exception(func + " command failure (CTRL/STAT reports some sticky flags)");
        if ( error & 0x10 ) throw nano::exception(func + " I/O failure (unexpected reply from shift-ir)");
        if ( error & 0xF0 ) throw nano::exception(func + " unknown error");

        const uint8_t ack = error & 0x0F;
        if ( ack == ACK_OKFAULT ) return /* ok */;
        if ( ack == ACK_WAIT ) throw nano::exception(func + " ACK_WAIT");
        throw nano::exception(func + " wrong ACK=" + std::to_string(ack));
    }

    uint32_t arm_xpacc(uint8_t ir, uint8_t reg, uint32_t value, bool write)
    {
        //printf("arm_xpacc(0x%02X, 0x%02X, 0x%08X, %s):\n", ir, reg, value, action(write));
        if ( reg > 0x0F ) throw nano::exception("arm_xpacc_ex() wrong register: " + std::to_string(reg));
        packet_t pkt;
        pkt.cmd = 9;
        pkt.len = 6;
        if ( pkt.len > PACKET_MAXLEN ) throw nano::exception("arm_xpacc() pkt.len too big: " + std::to_string(pkt.len));
        pkt.data[0] = ir;
        pkt.data[1] = (reg & 0xFC) | (write ? 0b00 : 0b10);
        pkt.data[2] = value & 0xFF;
        pkt.data[3] = (value >> 8) & 0xFF;
        pkt.data[4] = (value >> 16) & 0xFF;
        pkt.data[5] = (value >> 24) & 0xFF;

        //dump_packet("arm_xpacc() send", pkt);
        send_packet(pkt);

        recv_packet(pkt);
        //dump_packet("arm_xpacc() recv", pkt);

        if ( pkt.cmd != 9 ) throw nano::exception("arm_xpacc() wrong reply: " + std::to_string(pkt.cmd));
        if ( pkt.len != 6 ) throw nano::exception("arm_xpacc() wrong reply length: " + std::to_string(pkt.len));

        check_error("arm_xpacc()", pkt.data[1]);

        return pkt.data[2] | (pkt.data[3] << 8) | (pkt.data[4] << 16) | (pkt.data[5] << 24);
    }

    uint32_t arm_apacc(uint8_t ap, uint8_t reg, uint32_t value, bool write)
    {
        //printf("arm_apacc(ap=0x%02X, reg=0x%02X, value=0x%08X, %s):\n", ap, reg, value, action(write));
        if ( reg & 0x03 ) throw nano::exception("arm_apacc() wrong register: " + std::to_string(reg));
        packet_t pkt;
        pkt.cmd = 10;
        pkt.len = 6;
        if ( pkt.len > PACKET_MAXLEN ) throw nano::exception("arm_apacc() pkt.len too big: " + std::to_string(pkt.len));
        pkt.data[0] = ap;
        pkt.data[1] = (reg & 0xFC) | (write ? 0b00 : 0b10);
        pkt.data[2] = value & 0xFF;
        pkt.data[3] = (value >> 8) & 0xFF;
        pkt.data[4] = (value >> 16) & 0xFF;
        pkt.data[5] = (value >> 24) & 0xFF;

        //dump_packet("arm_apacc() send", pkt);
        send_packet(pkt);

        recv_packet(pkt);
        //dump_packet("arm_apacc() recv", pkt);

        if ( pkt.cmd != 10 ) throw nano::exception("arm_apacc() wrong reply: " + std::to_string(pkt.cmd));
        if ( pkt.len != 6 ) throw nano::exception("arm_apacc() wrong reply length: " + std::to_string(pkt.len));

        check_error("arm_apacc()", pkt.data[1]);

        return pkt.data[2] | (pkt.data[3] << 8) | (pkt.data[4] << 16) | (pkt.data[5] << 24);
    }

    void arm_set_memap(uint8_t ap)
    {
        printf("arm_set_memap(0x%02X)\n", ap);
        packet_t pkt;
        pkt.cmd = 11;
        pkt.len = 2;
        pkt.data[0] = 1;
        pkt.data[1] = ap;
        send_packet(pkt);
        recv_packet(pkt);
        if ( pkt.len != 2 || pkt.data[1] != ap ) throw nano::exception("arm_set_memap() failed");
    }

    void arm_set_memaddr(uint32_t addr)
    {
        //printf("arm_set_memaddr(0x%08X)\n", addr);
        packet_t pkt;
        pkt.cmd = 11;
        pkt.len = 5;
        pkt.data[0] = 2;
        pkt.data[1] = addr & 0xFF;
        pkt.data[2] = (addr >> 8) & 0xFF;
        pkt.data[3] = (addr >> 16) & 0xFF;
        pkt.data[4] = (addr >> 24) & 0xFF;
        //dump_packet("arm_set_memaddr() send:", pkt);
        send_packet(pkt);
        recv_packet(pkt);
        //dump_packet("arm_set_memaddr() recv:", pkt);
        if ( pkt.len != 5 ) throw nano::exception("arm_set_memaddr() wrong length: " + std::to_string(pkt.len));
        uint32_t output = pkt.data[1] | (pkt.data[2] << 8) | (pkt.data[3] << 16) | (pkt.data[4] << 24);
        if ( output != addr ) throw nano::exception("arm_set_memaddr() failed");
    }

    uint32_t arm_read_mem32()
    {
        //printf("arm_read_mem32()\n");
        packet_t pkt;
        pkt.cmd = 12;
        pkt.len = 4;
        send_packet(pkt);
        recv_packet(pkt);
        //dump_packet("arm_read_mem32() recv", pkt);
        if ( pkt.len != 4 ) throw nano::exception("arm_read_mem32() wrong length: " + std::to_string(pkt.len));
        return pkt.data[0] | (pkt.data[1] << 8) | (pkt.data[2] << 16) | (pkt.data[3] << 24);
    }

    uint16_t arm_read_mem16()
    {
        packet_t pkt;
        pkt.cmd = 12;
        pkt.len = 2;
        send_packet(pkt);
        recv_packet(pkt);
        if ( pkt.len != 2 ) throw nano::exception("arm_read_mem16() wrong length: " + std::to_string(pkt.len));
        return pkt.data[0] | (pkt.data[1] << 8);
    }

    void arm_write_mem32(uint32_t value)
    {
        packet_t pkt;
        pkt.cmd = 13;
        pkt.len = 4;
        pkt.data[0] = value & 0xFF;
        pkt.data[1] = (value >> 8) & 0xFF;
        pkt.data[2] = (value >> 16) & 0xFF;
        pkt.data[3] = (value >> 24) & 0xFF;
        send_packet(pkt);
        recv_packet(pkt);
        if ( pkt.len != 4 ) throw nano::exception("arm_write_mem32() wrong length: " + std::to_string(pkt.len));
        uint32_t output = pkt.data[0] | (pkt.data[1] << 8) | (pkt.data[2] << 16) | (pkt.data[3] << 24);
        if ( output != value ) throw nano::exception("arm_write_mem32() failed");
    }

    void arm_write_mem16(uint16_t value)
    {
        packet_t pkt;
        pkt.cmd = 13;
        pkt.len = 2;
        pkt.data[0] = value & 0xFF;
        pkt.data[1] = (value >> 8) & 0xFF;
        send_packet(pkt);
        recv_packet(pkt);
        if ( pkt.len != 2 ) throw nano::exception("arm_write_mem16() wrong length: " + std::to_string(pkt.len));
        uint16_t output = pkt.data[0] | (pkt.data[1] << 8);
        if ( output != value ) throw nano::exception("arm_write_mem16() failed");
    }

    void arm_fpec_program(uint32_t value)
    {
        packet_t pkt;
        pkt.cmd = 14;
        pkt.len = 4;
        pkt.data[0] = value & 0xFF;
        pkt.data[1] = (value >> 8) & 0xFF;
        pkt.data[2] = (value >> 16) & 0xFF;
        pkt.data[3] = (value >> 24) & 0xFF;
        //dump_packet("arm_fpec_program() send", pkt);
        send_packet(pkt);
        recv_packet(pkt);
        //dump_packet("arm_fpec_program() recv", pkt);
        if ( pkt.cmd != 14 ) throw nano::exception("arm_fpec_program() wrong cmd=" + std::to_string(pkt.cmd));
        if ( pkt.len == 2 )
        {
            printf("status0: 0x%02X, status1: 0x%02X\n", pkt.data[0], pkt.data[1]);
            throw nano::exception("arm_fpec_program() failed");
        }
        if ( pkt.len != 4 ) throw nano::exception("arm_fpec_program() wrong length: " + std::to_string(pkt.len));
        const uint32_t data = pkt.data[0] | (pkt.data[1] << 8) | (pkt.data[2] << 16) | (pkt.data[3] << 24);
        if ( data != value ) throw nano::exception("arm_fpec_program() failed");
    }

    uint32_t arm_read_dp(uint8_t addr)
    {
        return arm_xpacc(IR_DPACC, addr, 0, xpacc_read);
    }

    void arm_write_dp(uint8_t addr, uint32_t value)
    {
        arm_xpacc(IR_DPACC, addr, value, xpacc_write);
    }

    uint32_t arm_check_idcode()
    {
        //printf("\narm_check_idcode()\n\n");
        uint32_t idcode = arm_io<32>(IR_IDCODE, 0);
        const char *status = (idcode == CORTEX_M3_IDCODE) ? "[ ok ]" : "[fail]";
        printf("JTAG idcode: 0x%08X %s\n", idcode, status);
        return idcode;
    }

    uint32_t arm_status()
    {
        return arm_read_dp(0x4);
    }

    uint32_t arm_idr(uint8_t apsel)
    {
        return arm_apacc(apsel, 0xFC, 0, xpacc_read);
    }

    void arm_debug_enable()
    {
        //printf("arm_debug_enable()\n");

        cmd_jtag_reset();
        if ( arm_check_idcode() != CORTEX_M3_IDCODE ) throw nano::exception("only Cortex M3 supported yet");

        uint32_t status = CSYSPWRUPREQ | CDBGPWRUPREQ;
        arm_write_dp(0x4, status);

        status = arm_status();
        //printf("status: 0x%08X\n", status);

        if ( (status & CDBGPWRUPACK) == 0 )
        {
            throw nano::exception("no debug power");
        }
        if ( (status & CSYSPWRUPACK) == 0 )
        {
            throw nano::exception("no system power");
        }

        arm_find_memap();
    }

    void arm_debug_disable()
    {
        //printf("arm_debug_disable()\n");

        arm_write_dp(0x4, 0);

        uint32_t status = arm_status();
        //printf("status: 0x%08X\n", status);

        if ( (status & CDBGPWRUPACK) != 0 )
        {
            warn("debug power still on");
        }
        if ( (status & CSYSPWRUPACK) != 0 )
        {
            warn("system power still on");
        }
    }

    void arm_find_memap()
    {
        //printf("\nfind MEM-AP\n");
        for(int i = 0; i < 256; i++)
        {
            try
            {
                const uint32_t idr = arm_idr(i);
                const uint32_t test = (idr >> 16) & 0xFFF;
                //printf("idr(%d) = 0x%08X test=0x%03X\n", i, idr, test);
                if ( idr == 0 ) break;


                if ( test == 0x477 )
                {
                    arm_memap = i;
                    //printf("MEM-AP: %d\n", arm_memap);
                    return;
                }

            }
            catch(const nano::exception &e)
            {
                printf("idr(%d) error: %s\n", i, e.message().c_str());
                break;
            }

        }

        throw nano::exception("MEM-AP not found");

    }

    uint32_t arm_flash_size()
    {
        arm_set_memaddr(0x1FFFF7E0);
        return (arm_read_mem16() & 0xFFFF) * 1024;
    }

    uint32_t arm_fpec_read_reg(uint8_t reg)
    {
        arm_set_memaddr(0x40022000 + reg);
        return arm_read_mem32();
    }

    void arm_fpec_write_reg(uint8_t reg, const uint32_t &value)
    {
        arm_set_memaddr(0x40022000 + reg);
        arm_write_mem32(value);
    }

    void arm_fpec_unlock()
    {
        //printf("arm_fpec_unlock()\n");

        constexpr uint32_t KEY1 = 0x45670123;
        constexpr uint32_t KEY2 = 0xCDEF89AB;

        auto flash_cr = arm_fpec_read_reg(0x10);
        if ( (flash_cr & (1 << 7)) == 0 )
        {
            //printf("already unlocked\n");
            return;
        }

        arm_fpec_write_reg(0x04, KEY1);
        arm_fpec_write_reg(0x04, KEY2);

        flash_cr = arm_fpec_read_reg(0x10);
        if ( flash_cr & (1 << 7) )
        {
            throw nano::exception("arm_fpec_unlock() failed");
        }

        //printf("FPEC unlocked\n");
    }

    void arm_fpec_lock()
    {
        //printf("arm_fpec_lock()\n");
        arm_fpec_write_reg(0x10, (1 << 7)); // FLASH_CR_LOCK
    }

    void arm_fpec_reset_sr()
    {
        arm_fpec_write_reg(0x0C, (1 << 2) | (1 << 4) | (1 << 5));
    }

    void arm_fpec_check_sr()
    {
        uint32_t flash_sr = arm_fpec_read_reg(0x0C);

        if ( flash_sr & 1 )
        {
            printf("flash_sr: 0x%08X\n", flash_sr);
            throw nano::exception("FPEC is busy");
        }

        if ( flash_sr & (1 << 2) )
        {
            printf("flash_sr: 0x%08X\n", flash_sr);
            throw nano::exception("PGERR: Programming error (cell already programmed)");
        }

        if ( flash_sr & (1 << 4) )
        {
            printf("flash_sr: 0x%08X\n", flash_sr);
            throw nano::exception("WRPRTERR: Write protection error");
        }

        if ( flash_sr & (1 << 5) )
        {
            //printf("EOP: Flash operation (programming / erase) is completed\n");
            return;
        }

        printf("flash_sr: 0x%08X\n", flash_sr);
        printf("FPEC status unknown\n");
    }

    void arm_fpec_mass_erase()
    {
        printf("\narm_fpec_mass_erase()\n");
        arm_fpec_reset_sr();
        arm_fpec_write_reg(0x10, (1 << 2)); // FLASH_CR_MER
        arm_fpec_write_reg(0x10, (1 << 2) | (1 << 6)); // FLASH_CR_STRT
        arm_fpec_check_sr();
    }

    void arm_dump_mem32(uint32_t addr)
    {
        arm_set_memaddr(addr);
        const uint32_t value = arm_read_mem32();
        printf("MEM[0x%08X]: 0x%08X\n", addr, value);
    }

    uint32_t page_size() const override;
    uint32_t page_count() const override;

    static uint32_t parse_page_size(const std::string &ps)
    {
        if ( ps.empty() ) return 1024;

        const uint32_t size = nano::parse_size(ps);

        if ( size <= 0 )
        {
            warn("wrong page_size: " + ps);
            warn("page_size defaulted to 1024");
            return 1024;
        }

        if ( !nano::is_power_of_two(size) )
        {
            warn("page_size is not power of two: " + ps);
            warn("page_size defaulted to 1024");
            return 1024;
        }

        return size;
    }

    static uint32_t parse_flash_size(const std::string &fs, uint32_t page_size)
    {
        if ( fs.empty() )
        {
            throw nano::exception("specify flash_size");
        }

        const uint32_t size = nano::parse_size(fs);

        if ( size == 0 )
        {
            throw nano::exception("wrong flash_size: zero?");
        }

        if ( (size % page_size) != 0 )
        {
            throw nano::exception("wrong flash_size, should be multiple of page_size");
        }

        return size;
    }

    uint32_t flash_begin() const
    {
        return 0x08000000;
    }

    uint32_t flash_end() const
    {
        return flash_begin() + page_size() * page_count();
    }

    bool page_in_range(uint32_t page, uint32_t begin, uint32_t end)
    {
        return (page >= begin) && (page < end);
    }

    void show_info()
    {
        printf("flash_size: %dk\n", (arm.flash_size + 1023) / 1024);
        printf("page_size: %d\n", arm.page_size);
        printf("page_count: %d\n", arm.page_count);
        printf("\n");
    }

    /**
     * Проверить прошивку на корректность
     */
    bool check_firmware(const FirmwareData &pages, bool verbose)
    {
        bool status = true;
        const uint32_t page_begin = flash_begin();
        const uint32_t page_limit = flash_end();
        for(const auto page : pages)
        {
            const uint32_t page_addr = page.second.addr;
            const bool page_ok = page_in_range(page_addr, page_begin, page_limit);
            status = status && page_ok;
            if ( verbose )
            {
                const char *page_status = page_ok ? "ok" : "out of range [fail]";
                printf("PAGE[0x%08X] - %s\n", page_addr, page_status);
            }
        }
        if ( verbose )
        {
            const char *status_str = status ? "[ ok ]" : "[fail]";
            printf("overall status %s\n", status_str);
        }
        return status;
    }

    void action_test() override;
    void parse_device_info(const nano::options &) override;
    void isp_chip_info() override;
    void isp_stat_firmware(const FirmwareData &) override;
    void isp_check_firmware(const FirmwareData &) override;
    void isp_write_firmware(const FirmwareData &) override;
    void isp_chip_erase() override;
    void isp_read_fuse() override;
    void isp_write_fuse() override;

};

#endif // ARM_H
