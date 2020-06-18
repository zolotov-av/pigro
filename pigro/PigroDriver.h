#ifndef PIGRODRIVER_H
#define PIGRODRIVER_H

#include <cstdint>
#include <map>
#include <vector>

#include <nano/exception.h>

#include "DeviceInfo.h"
#include "PigroLink.h"

// TODO
uint32_t at_hex_to_int(const char *s);

class PigroDriver
{
protected:

    PigroLink *link;

public:

    static DeviceCode parseDeviceCode(const std::string &code)
    {
        if ( code.empty() ) throw nano::exception("wrong device code: " + code);
        const uint32_t hex = at_hex_to_int(code.c_str());
        const uint8_t b000 = hex >> 16;
        const uint8_t b001 = hex >> 8;
        const uint8_t b002 = hex;
        return {b000, b001, b002};
    }

    static std::optional<DeviceInfo> findDeviceByName(const std::string &name);

    struct PageData
    {
        uint16_t addr;
        std::vector<uint8_t> data;

        uint16_t page_size() const { return data.size() / 2; }
    };

    using FirmwareData = std::map<uint16_t, PageData>;

    PigroDriver(PigroLink *pl): link(pl) { }
    virtual ~PigroDriver() { }

    bool verbose() const { return link->verbose(); }

    std::string get_option(const std::string &name, const std::string &default_value = {})
    {
        return link->get_option(name, default_value);
    }

    const DeviceInfo& chip_info() const
    {
        return link->chip_info();
    }

    /**
     * Отправить пакет данных
     */
    bool send_packet(const packet_t &pkt)
    {
        return link->send_packet(&pkt);
    }

    /**
     * Прочитать пакет данных
     */
    void recv_packet(packet_t &pkt)
    {
        return link->recv_packet(&pkt);
    }

    void dump_packet(const char *message, const packet_t &pkt)
    {
        printf("%s [cmd=0x%02X]:", message, pkt.cmd);
        for(uint8_t i = 0; i < pkt.len; i++)
        {
            printf(" 0x%02X", pkt.data[i]);
        }
        printf("\n");
    }

    virtual void action_test() = 0;
    virtual void isp_chip_info() = 0;
    virtual void isp_check_firmware(const FirmwareData &) = 0;
    virtual void isp_write_firmware(const FirmwareData &) = 0;
    virtual void isp_chip_erase() = 0;
    virtual void isp_read_fuse() = 0;
    virtual void isp_write_fuse() = 0;
    virtual void isp_check_fuses() = 0;

};

#endif // PIGRODRIVER_H
