#ifndef PIGROAPP_H
#define PIGROAPP_H

#include <QThread>

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <atomic>

#include <array>
#include <nano/exception.h>
#include <nano/IniReader.h>
#include <nano/config.h>

#include "IntelHEX.h"
#include "AVR.h"
#include "ARM.h"
#include "DeviceInfo.h"
#include "PigroLink.h"
#include "PigroPrivate.h"


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

class PigroApp final: public QObject
{
    Q_OBJECT

private:

    static std::atomic<int> m_init_counter;

    QThread *m_thread { new QThread(this) };

    PigroPrivate *m_private { nullptr };

    bool m_verbose { false };
    bool nack_support { false };
    nano::config config;
    PigroLink *link { new PigroLink(this) };
    nano::options m_chip_info;
    std::string device_type;
    std::string hexfname;

    PigroDriver *driver = nullptr;

private slots:

    void threadStarted();
    void threadFinished();

public:

    uint8_t protoVersionMajor() const { return link->protoVersionMajor(); }
    uint8_t protoVersionMinor() const { return link->protoVersionMinor(); }

    QString protoVersion() const;

    explicit PigroApp(QObject *parent = nullptr);
    PigroApp(const PigroApp &) = delete;
    PigroApp(PigroApp &&) = delete;

    ~PigroApp();

    PigroApp& operator = (const PigroApp &) = delete;
    PigroApp& operator = (PigroApp &&) = delete;

    void open(const char *ttyPath, const char *configPath);

    bool verbose() const
    {
        return m_verbose;
    }

    void setVerbose(bool value)
    {
        m_verbose = value;
    }

    std::string get_option(const std::string &name, const std::string &default_value = {}) const
    {
        return config.value("main", name, default_value);
    }

    const nano::options& chip_info() const
    {
        return m_chip_info;
    }

    bool open(const QString &dev)
    {
        return link->open(dev);
    }

    void close()
    {
        link->close();
    }

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
        if ( name == "avr" ) return new AVR(link);
        if ( name == "arm" ) return new ARM(link);
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

    void start()
    {
        m_thread->start();
    }

    void stop()
    {
        m_thread->quit();
    }

    void wait()
    {
        m_thread->wait();
    }

signals:

    void started();
    void stopped();
    void sessionStarted(int major, int minor);

    void beginProgress(int min, int max);
    void reportProgress(int value);
    void reportMessage(const QString &message);
    void endProgress();

};

#endif // PIGROAPP_H
