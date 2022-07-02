#ifndef PIGROAPP_H
#define PIGROAPP_H

#include <QFile>
#include <QSerialPort>

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#include <array>
#include <nano/exception.h>
#include <nano/IniReader.h>
#include <nano/config.h>

#include "IntelHEX.h"
#include "AVR.h"
#include "ARM.h"
#include "DeviceInfo.h"
#include "PigroLink.h"


constexpr uint8_t PKT_ACK = 1;
constexpr uint8_t PKT_NACK = 2;

enum PigroAction {
    AT_ACT_INFO,
    AT_ACT_STAT,
    AT_ACT_CHECK,
    AT_ACT_WRITE,
    AT_ACT_ERASE,
    AT_ACT_READ_FUSE,
    AT_ACT_WRITE_FUSE,
    AT_ACT_TEST
};

class PigroApp: public QObject, public PigroLink
{
    Q_OBJECT

private:

    /**
     * Нужно ли выводить дополнительный отладочный вывод или быть тихим?
     */
    bool m_verbose { false };
    bool nack_support { false };
    uint8_t m_protoVersionMajor { 0 };
    uint8_t m_protoVersionMinor { 0 };
    nano::config config;
    QSerialPort *serial { new QSerialPort(this) };
    nano::options m_chip_info;
    std::string device_type;
    std::string hexfname;

    PigroDriver *driver = nullptr;

public:

    uint8_t protoVersionMajor() const { return m_protoVersionMajor; }
    uint8_t protoVersionMinor() const { return m_protoVersionMinor; }

    QString protoVersion() const;

    explicit PigroApp(QObject *parent = nullptr);
    PigroApp(const char *path, bool verbose);
    PigroApp(const PigroApp &) = delete;
    PigroApp(PigroApp &&) = delete;

    ~PigroApp();

    PigroApp& operator = (const PigroApp &) = delete;
    PigroApp& operator = (PigroApp &&) = delete;

    bool verbose() const override
    {
        return m_verbose;
    }

    std::string get_option(const std::string &name, const std::string &default_value = {}) override
    {
        return config.value("main", name, default_value);
    }

    const nano::options& chip_info() const override
    {
        return m_chip_info;
    }

    uint8_t read_sync()
    {
        char data;
        if ( serial->read(&data, 1) == 1 )
        {
            return data;
        }

        if ( serial->waitForReadyRead(200) )
        {
            char data;
            if ( serial->read(&data, 1) == 1 )
            {
                return data;
            }

            throw nano::exception("read fail: " + serial->errorString().toStdString());
        }
        else
        {
            // timeout
            throw nano::exception("read timeout");
        }
    }

    /**
     * Отправить пакет данных
     */
    bool send_packet(const packet_t *pkt) override
    {
        ssize_t r = serial->write(reinterpret_cast<const char *>(pkt), pkt->len + 2);
        serial->waitForBytesWritten(200);
        if ( r != pkt->len + 2 )
        {
            // TODO обработка ошибок
            throw nano::exception("send_packet(): send fail");
        }

        if ( nack_support )
        {
            switch ( read_sync() )
            {
            case PKT_ACK: return true;
            case PKT_NACK: throw nano::exception("send_packet(): NACK");
            default: throw nano::exception("send_packet(): out of sync");
            }
        }

        return true;
    }

    void recv_packet(packet_t *pkt) override
    {
        pkt->cmd = read_sync();
        pkt->len = read_sync();
        if ( pkt->len > PACKET_MAXLEN )
        {
            throw nano::exception("packet to big: " + std::to_string(pkt->len) + "/" + std::to_string((PACKET_MAXLEN)));
        }
        for(int i = 0; i < pkt->len; i++)
        {
            pkt->data[i] = read_sync();
        }
    }

    void beginProgress(int min, int max) override;
    void reportProgress(int value) override;
    void reportMessage(const QString &message) override;
    void endProcess() override;

    bool open(const QString &dev);
    void close();

    void info(const char *msg)
    {
        if ( verbose() )
        {
            printf("info: %s\n", msg);
        }
    }

    void warn(const char *msg)
    {
        fprintf(stderr, "warn: %s\n", msg);
    }

    void error(const char *msg)
    {
        fprintf(stderr, "error: %s\n", msg);
    }

    void checkProtoVersion()
    {
        serial->waitForReadyRead(200);
        serial->readAll();

        packet_t pkt;
        pkt.cmd = 1;
        pkt.len = 2;
        pkt.data[0] = 0;
        pkt.data[1] = 0;
        send_packet(&pkt);

        if ( serial->waitForReadyRead(200) )
        {
            char ack;
            serial->read(&ack, sizeof(ack));
            if ( ack == PKT_ACK )
            {
                recv_packet(&pkt);
                if ( pkt.len != 2 )
                    throw nano::exception("wrong protocol: len = " + std::to_string(pkt.len));
                nack_support = true;
                m_protoVersionMajor = pkt.data[0];
                m_protoVersionMinor = pkt.data[1];
                info("new protocol");
                return;
            }
            else
            {
                throw nano::exception("wrong protocol: ack = " + std::to_string(ack));
            }
        }

        nack_support = false;
        m_protoVersionMajor = 0;
        m_protoVersionMinor = 1;
        warn("old protocol? update firmware...");
    }

    void dumpProtoVersion()
    {
        if ( verbose() )
        {
            printf("proto version: %d.%d\n", m_protoVersionMajor, m_protoVersionMinor);
        }
    }

    /**
     * @brief set PORTA
     * @param value
     * @return
     */
    void cmd_seta(uint8_t value)
    {
        packet_t pkt;
        pkt.cmd = 1;
        pkt.len = 1;
        pkt.data[0] = value;
        send_packet(&pkt);
    }

    /**
     * @brief Действие - вывести информацию об устройстве
     * @return
     */
    int action_info()
    {
        loadConfig();

        driver->isp_chip_info();
        return 0;
    }

    QString getChipInfo()
    {
        return driver->getIspChipInfo();
    }

    FirmwareData readFirmware()
    {
        return driver->readFirmware();
    }

    bool checkFirmware(const QString &path)
    {
        return driver->checkFirmware(path);
    }

    /**
     * Действие - прочитать биты fuse
     */
    int action_read_fuse()
    {
        if ( verbose() )
        {
            info("read device's fuses");
        }

        loadConfig();

        driver->isp_read_fuse();
        return 0;
    }

    int action_write_fuse()
    {
        if ( verbose() )
        {
            info("write device's fuses");
        }

        loadConfig();

        driver->isp_write_fuse();
        return 0;
    }

    /**
     * Действие - стереть чип
     */
    int action_erase()
    {
        if ( verbose() )
        {
            info("erase device's frimware");
        }

        loadConfig();

        driver->isp_chip_erase();
        return 0;
    }

    PigroDriver* lookupDriver(const std::string &name)
    {
        if ( name == "avr" ) return new AVR(this);
        if ( name == "arm" ) return new ARM(this);
        throw nano::exception("unsupported driver: " + name);
    }

    void loadConfig();
    void loadConfig(const QString &path);

    FirmwareData readHEX()
    {
        loadConfig();

        auto pages = FirmwareData::LoadFromFile(hexfname, driver->page_size(), driver->page_fill());
        printf("page usages: %ld / %d\n", pages.size(), driver->page_count());
        return pages;
    }

    /**
     * Действие - проверить прошивку и вывести статистику (без обращения к чипу)
     */
    int action_stat()
    {
        FirmwareData pages = readHEX();
        driver->isp_stat_firmware(pages);
        return 0;
    }


    /**
     * Действие - проверить прошивку в устройстве
     */
    int action_check()
    {
        FirmwareData pages = readHEX();
        driver->isp_check_firmware(pages);
        return 0;
    }

    /**
     * Действие - записать прошивку в устройство
     */
    int action_write()
    {
        FirmwareData pages = readHEX();
        driver->isp_write_firmware(pages);
        return 0;
    }

    int action_test()
    {
        loadConfig();

        driver->action_test();
        return 0;
    }

    /**
     * Запус команды
     */
    int execute(PigroAction action)
    {
        switch ( action )
        {
        case AT_ACT_INFO: return action_info();
        case AT_ACT_STAT: return action_stat();
        case AT_ACT_CHECK: return action_check();
        case AT_ACT_WRITE: return action_write();
        case AT_ACT_ERASE: return action_erase();
        case AT_ACT_READ_FUSE: return action_read_fuse();
        case AT_ACT_WRITE_FUSE: return action_write_fuse();
        case AT_ACT_TEST: return action_test();
        default: throw nano::exception("Victory!");
        }
    }

signals:

    void beginProgress1(int min, int max);
    void reportProgress1(int value);
    void reportMessage1(const QString &message);
    void endProgress1();

};

#endif // PIGROAPP_H
