#ifndef PIGRODRIVER_H
#define PIGRODRIVER_H

#include <QFile>
#include <cstdint>
#include <map>
#include <vector>
#include <QString>

#include <nano/exception.h>
#include <nano/config.h>

#include "FirmwareData.h"
#include "PigroLink.h"

class PigroDriver
{
protected:

    PigroLink *link;

public:

    PigroDriver(PigroLink *pl): link(pl) { }
    virtual ~PigroDriver() { }

    bool verbose() const { return link->verbose(); }

    std::string get_option(const std::string &name, const std::string &default_value = {})
    {
        return link->get_option(name, default_value);
    }

    const nano::options& chip_info() const
    {
        return link->chip_info();
    }

    inline void info(const char *msg) const
    {
        if ( verbose() )
        {
            printf("info: %s\n", msg);
        }
    }

    static inline void warn(const char *msg)
    {
        fprintf(stderr, "warn: %s\n", msg);
    }

    static inline void warn(const std::string &msg)
    {
        warn( msg.c_str() );
    }

    inline void error(const char *msg) const
    {
        fprintf(stderr, "error: %s\n", msg);
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

    virtual uint32_t page_size() const = 0;
    virtual uint32_t page_count() const = 0;
    virtual uint8_t page_fill() const;

    virtual QString getIspChipInfo() = 0;
    virtual FirmwareData readFirmware() = 0;
    virtual bool checkFirmware(const QString &path) = 0;

    virtual void action_test() = 0;
    virtual void parse_device_info(const nano::options &info) = 0;
    virtual void isp_chip_info() = 0;
    virtual void isp_stat_firmware(const FirmwareData &) = 0;
    virtual void isp_check_firmware(const FirmwareData &) = 0;
    virtual void isp_write_firmware(const FirmwareData &) = 0;
    virtual void isp_chip_erase() = 0;
    virtual void isp_read_fuse() = 0;
    virtual void isp_write_fuse() = 0;

};

#endif // PIGRODRIVER_H
