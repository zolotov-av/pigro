#ifndef PIGRO_ARM_DRIVER_H
#define PIGRO_ARM_DRIVER_H

#include <cstdint>

#include <nano/string.h>
#include <nano/exception.h>
#include "PigroLink.h"
#include "PigroDriver.h"

class ARM: public PigroDriver
{
private:

    uint8_t memap;

public:

    static constexpr uint8_t IR_BYPASS = 0b1111;
    static constexpr uint8_t IR_IDCODE = 0b1110;
    static constexpr uint8_t IR_DPACC = 0b1010;
    static constexpr uint8_t IR_APACC = 0b1011;
    static constexpr uint8_t IR_ABORT = 0b1000;

    static constexpr uint8_t ACK_OKFAULT = 0b010;
    static constexpr uint8_t ACK_WAIT = 0b001;

    static constexpr uint32_t CORTEX_M3_IDCODE = 0x3BA00477;

    static constexpr bool is_cortex_m3_idcode(uint32_t idcode)
    {
        return (idcode & 0x0FFFFFFF) == (CORTEX_M3_IDCODE & 0x0FFFFFFF);
    }


    static constexpr uint32_t CSYSPWRUPACK = 1 << 31; // RO  System power-up acknowledge.
    static constexpr uint32_t CSYSPWRUPREQ = 1 << 30; // R/W System power-up request.
    static constexpr uint32_t CDBGPWRUPACK = 1 << 29; // Debug power-up acknowledge.
    static constexpr uint32_t CDBGPWRUPREQ = 1 << 28; // R/W Debug power-up request.

    static constexpr bool xpacc_read = false;
    static constexpr bool xpacc_write = true;

    struct ArmDeviceInfo
    {
        uint32_t page_size;
        uint32_t page_count;
        uint32_t flash_size;
    };

    ArmDeviceInfo arm;

    ARM(PigroLink *link): PigroDriver(link) { }
    ~ARM() override;

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

    void check_error(const std::string &func, const packet_t &pkt)
    {
        if ( pkt.len == 1 )
            check_error(func, pkt.data[0]);
    }

    template <uint8_t bitcount>
    static constexpr void write_bits(uint8_t *data, uint64_t value)
    {
        constexpr uint8_t bytecount = (bitcount + 7) / 8;
        for(uint8_t i = 0; i < bytecount; i++)
        {
            data[i] = value & 0xFF;
            value = value >> 8;
        }
    }

    template <uint8_t bitcount>
    static constexpr uint64_t read_bits(const uint8_t *data)
    {
        uint64_t result = 0;
        uint64_t mul = 1;
        constexpr uint8_t bytecount = (bitcount + 7) / 8;
        for(uint8_t i = 0; i < bytecount; i++)
        {
            result = result | (data[i] * mul);
            mul = mul * 256;
        }
        return result;
    }

    void cmd_jtag_reset(uint8_t value)
    {
        //printf("cmd_jtag_reset()\n");
        packet_t pkt;
        pkt.cmd = 5;
        pkt.len = 1;
        pkt.data[0] = value;
        auto status = send_packet(pkt);
        if ( !status ) throw nano::exception("cmd_jtag_reset() fail");
    }

    template <uint8_t bitcount>
    uint64_t cmd_jtag_raw_ir(uint64_t ir)
    {
        static_assert(bitcount <= 64);

        const unsigned byte_count = (bitcount + 7) / 8;
        static_assert(byte_count <= (PACKET_MAXLEN - 1));

        packet_t pkt;
        pkt.cmd = 6;
        pkt.len = byte_count + 1;
        pkt.data[0] = bitcount;
        write_bits<bitcount>(&pkt.data[1], ir);

        //dump_packet("cmd_jtag_raw_ir() send", pkt);
        send_packet(pkt);
        recv_packet(pkt);
        //dump_packet("cmd_jtag_raw_ir() recv", pkt);

        return read_bits<bitcount>(&pkt.data[1]);
    }

    template <uint8_t bitcount>
    uint64_t cmd_jtag_raw_dr(uint64_t dr)
    {
        static_assert(bitcount <= 64);

        const unsigned byte_count = (bitcount + 7) / 8;
        static_assert(byte_count <= (PACKET_MAXLEN - 1));

        packet_t pkt;
        pkt.cmd = 7;
        pkt.len = byte_count + 1;
        pkt.data[0] = bitcount;
        write_bits<bitcount>(&pkt.data[1], dr);

        //dump_packet("cmd_jtag_raw_dr() send", pkt);
        send_packet(pkt);
        recv_packet(pkt);
        //dump_packet("cmd_jtag_raw_dr() recv", pkt);

        return read_bits<bitcount>(&pkt.data[1]);
    }

    template <uint8_t bitcount>
    uint32_t cmd_raw_io(uint8_t ir, uint64_t value)
    {
        static_assert(bitcount <= 64);

        constexpr uint8_t bytecount = (bitcount + 7) / 8;
        static_assert(bytecount <= (PACKET_MAXLEN - 2));

        packet_t pkt;
        pkt.cmd = 8;
        pkt.len = bytecount + 2;
        pkt.data[0] = ir;
        pkt.data[1] = bitcount;
        write_bits<bitcount>(&pkt.data[2], value);

        //dump_packet("cmd_raw_io() send", pkt);
        send_packet(pkt);
        recv_packet(pkt);
        //dump_packet("cmd_raw_io() recv", pkt);

        if ( pkt.cmd != 8 ) throw nano::exception("cmd_raw_io() wrong reply: cmd=" + std::to_string(pkt.cmd));
        if ( pkt.len != bytecount + 2 ) throw nano::exception("cmd_raw_io() wrong len: " + std::to_string(pkt.len));

        return read_bits<bitcount>(&pkt.data[2]);
    }

    uint32_t cmd_xpacc(uint8_t ir, uint8_t reg, uint32_t value, bool write)
    {
        //printf("cmd_xpacc(0x%02X, 0x%02X, 0x%08X, %s):\n", ir, reg, value, action(write));
        if ( reg > 0x0F ) throw nano::exception("cmd_xpacc() wrong register: " + std::to_string(reg));

        packet_t pkt;
        pkt.cmd = 9;
        pkt.len = 6;
        pkt.data[0] = ir;
        pkt.data[1] = (reg & 0xFC) | (write ? 0b00 : 0b10);
        write_bits<32>(&pkt.data[2], value);

        //dump_packet("cmd_xpacc() send", pkt);
        send_packet(pkt);
        recv_packet(pkt);
        //dump_packet("cmd_xpacc() recv", pkt);

        check_error("cmd_xpacc()", pkt);

        if ( pkt.cmd != 9 ) throw nano::exception("cmd_xpacc() wrong reply: " + std::to_string(pkt.cmd));
        if ( pkt.len != 6 ) throw nano::exception("cmd_xpacc() wrong reply length: " + std::to_string(pkt.len));

        return read_bits<32>(&pkt.data[2]);
    }

    uint32_t cmd_apacc(uint8_t ap, uint8_t reg, uint32_t value, bool write)
    {
        //printf("cmd_apacc(ap=0x%02X, reg=0x%02X, value=0x%08X, %s):\n", ap, reg, value, action(write));
        if ( reg & 0x03 ) throw nano::exception("cmd_apacc() wrong register: " + std::to_string(reg));

        packet_t pkt;
        pkt.cmd = 10;
        pkt.len = 6;
        pkt.data[0] = ap;
        pkt.data[1] = (reg & 0xFC) | (write ? 0b00 : 0b10);
        write_bits<32>(&pkt.data[2], value);

        //dump_packet("cmd_apacc() send", pkt);
        send_packet(pkt);
        recv_packet(pkt);
        //dump_packet("cmd_apacc() recv", pkt);

        check_error("cmd_apacc()", pkt);

        if ( pkt.cmd != 10 ) throw nano::exception("cmd_apacc() wrong reply: " + std::to_string(pkt.cmd));
        if ( pkt.len != 6 ) throw nano::exception("cmd_apacc() wrong reply length: " + std::to_string(pkt.len));

        return read_bits<32>(&pkt.data[2]);
    }

    template <uint8_t bitcount>
    uint64_t cmd_config(uint8_t param, uint64_t value, [[maybe_unused]] const char *func = "cmd_config()")
    {
        constexpr uint8_t bytecount = (bitcount + 7) / 8;
        //printf("cmd_config(0x%02X, 0x%lX)\n", param, value);
        packet_t pkt;
        pkt.cmd = 11;
        pkt.len = bytecount + 1;
        pkt.data[0] = param;
        write_bits<bitcount>(&pkt.data[1], value);
        //dump_packet(func, pkt);
        send_packet(pkt);
        recv_packet(pkt);
        //dump_packet(func, pkt);
        if ( pkt.len != (bytecount + 1) ) throw nano::exception(std::string(func) + " wrong len: " + std::to_string(pkt.len));
        return read_bits<bitcount>(&pkt.data[1]);
    }

    void set_memap(uint8_t ap)
    {
        printf("set_memap(0x%02X)\n", ap);
        const uint8_t result = cmd_config<8>(1, ap, "set_memap()");
        if ( result != ap ) throw nano::exception("set_memap() failed");
    }

    void set_memaddr(uint32_t addr)
    {
        //printf("set_memaddr(0x%08X)\n", addr);
        uint32_t output = cmd_config<32>(2, addr, "set_memaddr()");
        if ( output != addr ) throw nano::exception("set_memaddr() failed");
    }

    template <uint8_t bitcount>
    uint64_t read_next()
    {
        constexpr uint8_t bytecount = (bitcount + 7) / 8;
        packet_t pkt;
        pkt.cmd = 12;
        pkt.len = bytecount;
        send_packet(pkt);
        recv_packet(pkt);
        check_error("read_next()", pkt);
        if ( pkt.len != bytecount ) throw nano::exception("read_next() wrong length: " + std::to_string(pkt.len));
        return read_bits<bitcount>(&pkt.data[0]);
    }

    inline uint32_t read_next32()
    {
        return read_next<32>();
    }

    inline uint16_t read_next16()
    {
        return read_next<16>();
    }

    template <uint8_t bitcount>
    void write_next(uint64_t value)
    {
        constexpr uint8_t bytecount = (bitcount + 7) / 8;
        packet_t pkt;
        pkt.cmd = 13;
        pkt.len = bytecount;
        write_bits<bitcount>(&pkt.data[0], value);
        send_packet(pkt);
        recv_packet(pkt);
        check_error("write_next()", pkt);
        if ( pkt.len != bytecount ) throw nano::exception("write_next() wrong length: " + std::to_string(pkt.len));
        const uint64_t output = read_bits<bitcount>(&pkt.data[0]);
        if ( output != value ) throw nano::exception("write_next() failed");
    }

    inline void write_next32(uint32_t value)
    {
        return write_next<32>(value);
    }

    inline void write_next16(uint16_t value)
    {
        return write_next<16>(value);
    }

    void cmd_program_next(uint32_t value)
    {
        packet_t pkt;
        pkt.cmd = 14;
        pkt.len = 4;
        write_bits<32>(&pkt.data[0], value);
        //dump_packet("cmd_program_next() send", pkt);
        send_packet(pkt);
        recv_packet(pkt);
        //dump_packet("cmd_program_next() recv", pkt);
        if ( pkt.cmd != 14 ) throw nano::exception("cmd_program_next() wrong cmd=" + std::to_string(pkt.cmd));
        if ( pkt.len == 2 )
        {
            printf("cmd_program_next() status0: 0x%02X, status1: 0x%02X\n", pkt.data[0], pkt.data[1]);
            throw nano::exception("cmd_program_next() failed");
        }
        if ( pkt.len != 4 ) throw nano::exception("cmd_program_next() wrong length: " + std::to_string(pkt.len));
        const uint16_t output0 = read_bits<16>(&pkt.data[0]);
        const uint16_t output1 = read_bits<16>(&pkt.data[2]);
        const uint32_t output = output0 | (uint32_t(output1) << 16);
        if ( output != value ) throw nano::exception("cmd_program_next() failed");
    }

    template <uint8_t bitcount>
    uint64_t read_mem(uint32_t addr)
    {
        constexpr uint8_t bytecount = (bitcount + 7) / 8;
        packet_t pkt;
        pkt.cmd = 15;
        pkt.len = 4 + bytecount;
        write_bits<32>(&pkt.data[0], addr);
        write_bits<bitcount>(&pkt.data[4], 0);

        send_packet(pkt);
        recv_packet(pkt);

        if ( pkt.cmd != 15 ) throw nano::exception("read_mem() wrong cmd = " + std::to_string(pkt.cmd));
        if ( pkt.len != (bytecount + 4) ) throw nano::exception("read_mem() wrong len = " + std::to_string(pkt.len));

        return read_bits<bitcount>(&pkt.data[4]);
    }

    inline uint32_t read_mem32(uint32_t addr)
    {
        return read_mem<32>(addr);
    }

    inline uint16_t read_mem16(uint32_t addr)
    {
        return read_mem<16>(addr);
    }

    template <uint8_t bitcount>
    void write_mem(uint32_t addr, uint64_t value)
    {
        constexpr uint8_t bytecount = (bitcount + 7) / 8;
        packet_t pkt;
        pkt.cmd = 16;
        pkt.len = 4 + bytecount;
        write_bits<32>(&pkt.data[0], addr);
        write_bits<bitcount>(&pkt.data[4], value);

        //dump_packet("write_mem32() send", pkt);
        send_packet(pkt);
        recv_packet(pkt);
        //dump_packet("write_mem32() recv", pkt);

        if ( pkt.cmd != 16 ) throw nano::exception("write_mem32(): wrong cmd = " + std::to_string(pkt.cmd));
        if ( pkt.len != (bytecount + 4) ) throw nano::exception("write_mem32(): wrong len = " + std::to_string(pkt.len));
    }

    inline void write_mem32(uint32_t addr, uint32_t value)
    {
        return write_mem<32>(addr, value);
    }

    inline void write_mem16(uint32_t addr, uint32_t value)
    {
        return write_mem<16>(addr, value);
    }

    inline uint32_t read_dp(uint8_t addr)
    {
        return cmd_xpacc(IR_DPACC, addr, 0, xpacc_read);
    }

    inline void write_dp(uint8_t addr, uint32_t value)
    {
        cmd_xpacc(IR_DPACC, addr, value, xpacc_write);
    }

    inline uint32_t read_ap(uint8_t ap, uint8_t reg)
    {
        return cmd_apacc(ap, reg, 0, xpacc_read);
    }

    inline void write_ap(uint8_t ap, uint8_t reg, uint32_t value)
    {
        cmd_apacc(ap, reg, value, xpacc_write);
    }

    uint32_t check_idcode()
    {
        //printf("\ncheck_idcode()\n\n");
        uint32_t idcode = cmd_raw_io<32>(IR_IDCODE, 0);
        const char *status = is_cortex_m3_idcode(idcode) ? "[ ok ]" : "[fail]";
        printf("JTAG idcode: 0x%08X %s\n", idcode, status);
        return idcode;
    }

    uint32_t check_idcode_raw()
    {
        printf("\ncheck_idcode_raw()\n");

        cmd_jtag_reset(0);
        const uint64_t idcode = cmd_jtag_raw_dr<64>(0);

        const uint32_t stm32_id = idcode & 0xFFFFFFFF;
        const uint32_t tap_id = (idcode >> 32) & 0xFFFFFFFF;

        const char *status = is_cortex_m3_idcode(stm32_id) ? "[ ok ]" : "[fail]";
        printf("  STM32 idcode: 0x%08X %s\n", stm32_id, status);
        printf("    TAP idcode: 0x%08X\n", tap_id);
        return stm32_id;
    }

    template <uint8_t ir_count, uint8_t bitcount>
    uint64_t check_bypass(uint64_t value)
    {
        const uint32_t ir_ack = cmd_jtag_raw_ir<ir_count>(-1);
        const uint64_t result = cmd_jtag_raw_dr<bitcount>(value);
        constexpr uint64_t mask = (uint64_t(1) << bitcount) - 1;
        const uint64_t test_value = (value << 2) & mask;
        const char *status = (result == test_value) ? "[ ok ]" : "[fail]";
        printf("\ncheck_bypass<%d, %d>(0x%016lX): ir = 0x%03X, data = 0x%016lX %s\n", ir_count, bitcount, value, ir_ack, result, status);
        return result;
    }

    inline uint32_t dp_status()
    {
        return read_dp(0x4);
    }

    uint32_t ap_idr(uint8_t ap)
    {
        return read_ap(ap, 0xFC);
    }

    void debug_enable()
    {
        //printf("debug_enable()\n");

        cmd_jtag_reset(0);
        cmd_jtag_reset(2);

        if ( !is_cortex_m3_idcode( check_idcode() ) ) throw nano::exception("only Cortex M3 supported yet");

        uint32_t status = CSYSPWRUPREQ | CDBGPWRUPREQ;
        write_dp(0x4, status);

        status = dp_status();
        //printf("status: 0x%08X\n", status);

        if ( (status & CDBGPWRUPACK) == 0 )
        {
            throw nano::exception("no debug power");
        }
        if ( (status & CSYSPWRUPACK) == 0 )
        {
            throw nano::exception("no system power");
        }

        find_memap();


        uint32_t test;

        write_mem32(0xE000EDF0, 0xA05F0003);

        test = read_mem32(0xE000EDF0);
        printf("DHCSR: 0x%08X\n", test);

        write_mem32(0xE000EDFC, 1);

        test = read_mem32(0xE000EDFC);
        printf("DEMCR: 0x%08X\n", test);

        cmd_jtag_reset(1);

        test = read_mem32(0xE000EDF0);
        printf("DHCSR: 0x%08X\n", test);

        test = read_mem32(0xE000EDF0);
        printf("DHCSR: 0x%08X\n", test);

        test = read_mem32(0xE000ED10);
        printf("SCR: 0x%08X\n", test);


        //
    }

    void debug_disable()
    {
        //printf("debug_disable()\n");

        write_dp(0x4, 0);

        uint32_t status = dp_status();
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

    void find_memap()
    {
        //printf("\nfind MEM-AP\n");
        for(int i = 0; i < 256; i++)
        {
            try
            {
                const uint32_t idr = ap_idr(i);
                const uint32_t test = (idr >> 16) & 0xFFF;
                //printf("idr(%d) = 0x%08X test=0x%03X\n", i, idr, test);
                if ( idr == 0 ) break;


                if ( test == 0x477 )
                {
                    memap = i;
                    //printf("MEM-AP: %d\n", memap);
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

    uint32_t read_flash_size()
    {
        return read_mem16(0x1FFFF7E0) * 1024;
    }

    uint32_t read_fpec(uint8_t reg)
    {
        return read_mem32(0x40022000 + reg);
    }

    void write_fpec(uint8_t reg, const uint32_t &value)
    {
        return write_mem32(0x40022000 + reg, value);
    }

    void unlock_fpec()
    {
        //printf("unlock_fpec()\n");

        constexpr uint32_t KEY1 = 0x45670123;
        constexpr uint32_t KEY2 = 0xCDEF89AB;

        auto flash_cr = read_fpec(0x10);
        if ( (flash_cr & (1 << 7)) == 0 )
        {
            //printf("already unlocked\n");
            return;
        }

        write_fpec(0x04, KEY1);
        write_fpec(0x04, KEY2);

        flash_cr = read_fpec(0x10);
        if ( flash_cr & (1 << 7) )
        {
            throw nano::exception("unlock_fpec() failed");
        }

        //printf("FPEC unlocked\n");
    }

    void lock_fpec()
    {
        //printf("lock_fpec()\n");
        write_fpec(0x10, (1 << 7)); // FLASH_CR_LOCK
    }

    /**
     * Reset errors in FLASH_SR register
     */
    void reset_flash_sr()
    {
        write_fpec(0x0C, (1 << 2) | (1 << 4) | (1 << 5));
    }

    /**
     * Check errors in FLASH_SR register
     */
    void check_flash_sr()
    {
        uint32_t flash_sr = read_fpec(0x0C);

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

    void fpec_mass_erase()
    {
        printf("\nfpec_mass_erase()\n");
        reset_flash_sr();
        write_fpec(0x10, (1 << 2)); // FLASH_CR_MER
        write_fpec(0x10, (1 << 2) | (1 << 6)); // FLASH_CR_STRT
        check_flash_sr();
    }

    void dump_mem32(uint32_t addr)
    {
        const uint32_t value = read_mem32(addr);
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

#endif // PIGRO_ARM_DRIVER_H
