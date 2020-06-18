#ifndef AVR_H
#define AVR_H

#include <cstdint>
#include <array>
#include <vector>
#include <map>

#include <nano/exception.h>

#include "DeviceInfo.h"
#include "PigroLink.h"
#include "PigroDriver.h"

/**
 * Конверировать шестнадцатеричную цифру в число
 */
inline uint8_t at_hex_digit(char ch)
{
    if ( ch >= '0' && ch <= '9' ) return ch - '0';
    if ( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
    if ( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
    throw nano::exception("wrong hex digit");
}

/**
 * Первести шестнадцатеричное число из строки в целочисленное значение
 */
inline uint32_t parse_hexint(const char *s)
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

inline uint8_t parse_fuse(const std::string &s, const char *type)
{
    uint32_t value = parse_hexint(s.c_str());
    if ( value > 0xFF ) throw nano::exception(std::string("wrong ") + type + ": " + s);
    return value & 0xFF;
}

class AVR: public PigroDriver
{
public:

    DeviceInfo avr;

    AVR(PigroLink *link): PigroDriver(link) { }
    ~AVR() override;

    /**
     * @brief Управление линией RESET программируемого контроллера
     * @param value
     * @return
     */
    void cmd_isp_reset(bool value)
    {
        packet_t pkt;
        pkt.cmd = 2;
        pkt.len = 1;
        pkt.data[0] = value ? 1 : 0;
        send_packet(pkt);
    }

    /**
     * Отправить команду программирования (ISP)
     */
    unsigned int cmd_isp_io(unsigned int cmd)
    {
        packet_t pkt;
        pkt.cmd = 3;
        pkt.len = 4;

        for(int i = 0; i < 4; i++)
        {
            unsigned char byte = cmd >> 24;
            pkt.data[i] = byte;
            cmd <<= 8;
        }

        send_packet(pkt);

        recv_packet(pkt);
        if ( pkt.cmd != 3 || pkt.len != 4 ) throw nano::exception("unexpected packet");

        unsigned int result = 0;
        for(int i = 0; i < 4; i++)
        {
            unsigned char byte = pkt.data[i];
            result = result * 256 + byte;
        }

        return result;
    }

    /**
     * @brief Подать сигнал RESET и командду "Programming Enable"
     * @return
     */
    int isp_program_enable()
    {
        cmd_isp_reset(0);
        cmd_isp_reset(1);
        cmd_isp_reset(0);

        unsigned int r = cmd_isp_io(0xAC530000);
        int status = (r & 0xFF00) == 0x5300;
        if ( /* verbose || */ !status )
        {
            const char *s = status ? "ok" : "fault";
            printf("at_program_enable(): %s 0x%08x\n", s, r);
        }
        if ( !status )
        {
            throw nano::exception("isp_program_enable() failed");
        }
        return status;
    }

    /**
     * Завершить программирование
     */
    void isp_program_disable()
    {
        cmd_isp_reset(1);
    }

    /**
     * Подать команду "Read Device Code"
     *
     *
     * Лучший способ проверить связь с чипом в режиме программирования.
     * Эта команда возвращает код который индентфицирует модель чипа.
     */
    DeviceCode isp_read_chip_info()
    {
        uint8_t b000 = cmd_isp_io(0x30000000) & 0xFF;
        uint8_t b001 = cmd_isp_io(0x30000100) & 0xFF;
        uint8_t b002 = cmd_isp_io(0x30000200) & 0xFF;

        return {b000, b001, b002};
    }

    static DeviceCode parseDeviceCode(const std::string &code)
    {
        if ( code.empty() ) throw nano::exception("wrong device code: " + code);
        const uint32_t hex = at_hex_to_int(code.c_str());
        const uint8_t b000 = hex >> 16;
        const uint8_t b001 = hex >> 8;
        const uint8_t b002 = hex;
        return {b000, b001, b002};
    }

    bool check_chip_info()
    {
        auto info = isp_read_chip_info();
        bool status = (info == parseDeviceCode(chip_info().value("device_code")));
        const char *status_str = status ? "[ ok ]" : "[diff]";
        printf("chip signature: 0x%02X, 0x%02X, 0x%02X %s\n", info[0], info[1], info[2], status_str);
        return status;
    }

    bool chip_erase()
    {
        unsigned int r = cmd_isp_io(0xAC800000);
        bool status = ((r >> 16) & 0xFF) == 0xAC;
        if ( !status ) error("isp_chip_erase() error");
        return status;
    }

    /**
     * Прочитать байт прошивки из устройства
     */
    uint8_t isp_read_memory(uint16_t addr)
    {
        uint8_t cmd = (addr & 1) ? 0x28 : 0x20;
        uint16_t offset = (addr >> 1);
        return cmd_isp_io( (cmd << 24) | (offset << 8 ) ) & 0xFF;
    }

    /**
     * Загрузить байт прошивки в буфер страницы
     */
    void isp_load_memory_page(uint16_t addr, uint8_t byte)
    {
        uint8_t cmd = (addr & 1) ? 0x48 : 0x40;
        //uint16_t offset = (addr >> 1);
        uint16_t word_addr = (addr >> 1);
        uint32_t result = cmd_isp_io( (cmd << 24) | (word_addr << 8 ) | (byte & 0xFF) );
        uint8_t r = (result >> 16) & 0xFF;
        int status = (r == cmd);
        if ( !status ) throw nano::exception("isp_load_memory_page() error");
    }

    /**
     * Записать буфер страницы
     */
    int isp_write_memory_page(uint16_t page_addr)
    {
        uint8_t cmd = 0x4C;
        uint32_t result = cmd_isp_io( (cmd << 24) | (page_addr << 8 ) );
        uint8_t r = (result >> 16) & 0xFF;
        int status = (r == cmd);
        //sleep(1);
        return status;
    }

    uint8_t isp_read_fuse_high()
    {
        uint32_t r = cmd_isp_io(0x58080000);
        uint8_t echo = (r >> 8) & 0xFF;
        if ( echo != 0x08 ) throw nano::exception("isp_read_fuse_high() out of sync");

        return r & 0xFF;
    }

    uint8_t isp_read_fuse_low()
    {
        uint32_t r = cmd_isp_io(0x50000000);
        uint8_t echo = (r >> 8) & 0xFF;
        if ( echo != 0x00 ) throw nano::exception("isp_read_fuse_low() out of sync");

        return r & 0xFF;
    }

    uint8_t isp_read_fuse_ext()
    {
        uint32_t r = cmd_isp_io(0x50080000);
        uint8_t echo = (r >> 8) & 0xFF;
        if ( echo != 0x08 ) throw nano::exception("isp_read_fuse_ext() out of sync");

        return r & 0xFF;
    }

    void isp_write_fuse_low(uint8_t value)
    {
        unsigned int r = cmd_isp_io(0xACA00000 + (value & 0xFF));
        int ok = ((r >> 16) & 0xFF) == 0xAC;

        if ( verbose() || !ok )
        {
            const char *status = ok ? "[ ok ]" : "[ fail ]";
            printf("write device's low fuse bits %s\n", status);
        }
    }

    void isp_write_fuse_high(uint8_t value)
    {
        uint32_t r = cmd_isp_io(0xACA80000 + (value & 0xFF));
        int ok = ((r >> 8) & 0xFF) == 0xA8;

        if ( verbose() || !ok )
        {
            const char *status = ok ? "[ ok ]" : "[ fail ]";
            printf("write device's high fuse bits %s\n", status);
        }
    }

    void isp_write_fuse_ext(uint8_t value)
    {
        uint32_t r = cmd_isp_io(0xACA40000 + (value & 0xFF));
        int ok = ((r >> 8) & 0xFF) == 0xA4;

        if ( verbose() || !ok )
        {
            const char *status = ok ? "[ ok ]" : "[ fail ]";
            printf("write device's extended fuse bits %s\n", status);
        }
    }

    void check_fuses()
    {
        if ( auto s = get_option("fuse_low"); !s.empty() )
        {
            const uint8_t fuse_lo = isp_read_fuse_low();
            const uint8_t x = parse_fuse(s, "fuse_low (pigro.ini)");
            const char *status = (x == fuse_lo) ? " ok " : "diff";
            printf("fuse low:  0x%02X [%s]\n", fuse_lo, status);
        }

        if ( auto s = get_option("fuse_high"); !s.empty() )
        {
            const uint8_t fuse_hi = isp_read_fuse_high();
            const uint8_t x = parse_fuse(s, "fuse_high (pigro.ini)");
            const char *status = (x == fuse_hi) ? " ok " : "diff";
            printf("fuse high: 0x%02X [%s]\n", fuse_hi, status);
        }

        if ( auto s = get_option("fuse_ext"); !s.empty() )
        {
            const uint8_t fuse_ext = isp_read_fuse_ext();
            const uint8_t x = parse_fuse(s, "fuse_ext (pigro.ini)");
            const char *status = (x == fuse_ext) ? " ok " : "diff";
            printf("fuse ext:  0x%02X [%s]\n", fuse_ext, status);
        }
    }

    uint32_t page_size() const override;
    uint32_t page_count() const override;

    void action_test() override;
    void parse_device_info(const nano::options &info) override;
    void isp_chip_info() override;
    void isp_check_firmware(const FirmwareData &) override;
    void isp_write_firmware(const FirmwareData &) override;
    void isp_chip_erase() override;
    void isp_read_fuse() override;
    void isp_write_fuse() override;

};

using AVR_Info = DeviceInfo;

using AVR_Page = PigroDriver::PageData;

using AVR_Data = PigroDriver::FirmwareData;

AVR::FirmwareData avr_load_from_hex(uint32_t page_size, const std::string &path);

#endif // AVR_H
