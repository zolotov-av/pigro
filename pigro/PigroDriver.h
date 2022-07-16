#ifndef PIGRODRIVER_H
#define PIGRODRIVER_H

#include <QFile>
#include <cstdint>
#include <map>
#include <vector>
#include <atomic>
#include <QString>

#include <nano/exception.h>
#include <nano/config.h>

#include "FirmwareInfo.h"
#include "FirmwareData.h"
#include "PigroLink.h"

class Pigro;

class PigroDriver: public QObject
{
    Q_OBJECT

private:

    Pigro *m_owner {nullptr};
    PigroLink *m_link {nullptr};

    FirmwareInfo m_firmware_info;

protected:

    std::atomic<bool> m_cancel {false};

    std::string get_option(const std::string &name, const std::string &default_value = {}) const
    {
        return m_firmware_info.projectInfo.value(name, default_value);
    }

    const nano::options& chip_info() const
    {
        return m_firmware_info.m_chip_info;
    }

    void info(const char *msg) const;
    void warn(const char *msg);
    void warn(const std::string &msg)
    {
        warn( msg.c_str() );
    }

    void error(const char *msg) const;

    /**
     * Отправить пакет данных
     */
    bool send_packet(const packet_t &pkt)
    {
        return m_link->send_packet(&pkt);
    }

    /**
     * Прочитать пакет данных
     */
    void recv_packet(packet_t &pkt)
    {
        return m_link->recv_packet(&pkt);
    }

    static void dump_packet(const char *message, const packet_t &pkt)
    {
        printf("%s [cmd=0x%02X]:", message, pkt.cmd);
        for(uint8_t i = 0; i < pkt.len; i++)
        {
            printf(" 0x%02X", pkt.data[i]);
        }
        printf("\n");
    }

    void beginProgress(int min, int max);
    void reportProgress(int value);
    void reportMessage(const QString &message);
    void reportResult(const QString &result);
    void endProgress();

public:

    bool verbose() const
    {
        return m_firmware_info.verbose;
    }

    const FirmwareInfo& firmwareInfo() const
    {
        return m_firmware_info;
    }

    PigroDriver(PigroLink *link, Pigro *owner);
    PigroDriver(const PigroDriver &) = delete;
    PigroDriver(PigroDriver &&) = delete;

    virtual ~PigroDriver();

    PigroDriver& operator = (const PigroDriver &) = delete;
    PigroDriver& operator = (PigroDriver &&) = delete;

    void setVerbose(bool value)
    {
        m_firmware_info.verbose = value;
    }

    void setFirmwareInfo(const FirmwareInfo &info)
    {
        m_firmware_info = info;
    }

    void cancel();

    virtual uint32_t page_size() const = 0;
    virtual uint32_t page_count() const = 0;
    virtual uint8_t page_fill() const;

    virtual QString getIspChipInfo() = 0;
    virtual FirmwareData readFirmware() = 0;

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
