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
#include "FirmwareInfo.h"


class PigroApp final: public QObject
{
    Q_OBJECT

private:

    static std::atomic<int> m_init_counter;

    QThread *m_thread { new QThread(this) };
    PigroPrivate *m_private { nullptr };
    PigroLink *link { new PigroLink(this) };
    PigroDriver *driver = nullptr;

private slots:

    void threadStarted();
    void threadFinished();

public:

    QString protoVersion() const { return  link->protoVersion(); }
    uint8_t protoVersionMajor() const { return link->protoVersionMajor(); }
    uint8_t protoVersionMinor() const { return link->protoVersionMinor(); }

    explicit PigroApp(QObject *parent = nullptr);
    PigroApp(const PigroApp &) = delete;
    PigroApp(PigroApp &&) = delete;

    ~PigroApp();

    PigroApp& operator = (const PigroApp &) = delete;
    PigroApp& operator = (PigroApp &&) = delete;

    void open(const char *ttyPath, const char *configPath);

    bool verbose() const
    {
        return link->verbose();
    }

    void setVerbose(bool value)
    {
        link->setVerbose(value);
    }

    void setTTY(const QString &device)
    {
        link->setTTY(device);
    }

    void setProjectPath(const QString &path)
    {
        link->setProjectPath(path);
    }

    bool open(const QString &dev)
    {
        link->setTTY(dev);
        return link->open();
    }

    bool open()
    {
        return link->open();
    }

    void close()
    {
        if ( driver )
        {
            delete driver;
            driver = nullptr;
        }
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

    QString getChipInfo()
    {
        return driver->getIspChipInfo();
    }

    void execChipInfo()
    {
        QMetaObject::invokeMethod(m_private, "execChipInfo", Q_ARG(QString, link->tty()), Q_ARG(QString, link->projectPath()));
    }

    void execCheckFirmware()
    {
        QMetaObject::invokeMethod(m_private, "checkFirmware", Q_ARG(QString, link->tty()), Q_ARG(QString, link->projectPath()));
    }

    void execChipErase()
    {
        QMetaObject::invokeMethod(m_private, "chipErase", Q_ARG(QString, link->tty()), Q_ARG(QString, link->projectPath()));
    }

    void execWriteFirmware()
    {
        QMetaObject::invokeMethod(m_private, "writeFirmware", Q_ARG(QString, link->tty()), Q_ARG(QString, link->projectPath()));
    }

    void execWriteFuse()
    {
        QMetaObject::invokeMethod(m_private, "writeFuse", Q_ARG(QString, link->tty()), Q_ARG(QString, link->projectPath()));
    }

    FirmwareData readFirmware()
    {
        return driver->readFirmware();
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

    PigroDriver* lookupDriver(const FirmwareInfo &name)
    {
        const auto driver = lookupDriver(name.device_type);
        driver->parse_device_info(name.m_chip_info);
        return driver;
    }

    void loadConfig();
    void loadConfig(const QString &path);

    FirmwareData readHEX()
    {
        loadConfig();

        auto pages = FirmwareData::LoadFromFile(link->firmwareInfo().hexFilePath.toStdString(), driver->page_size(), driver->page_fill());
        printf("page usages: %ld / %d\n", pages.size(), driver->page_count());
        return pages;
    }

    /**
     * @brief Действие - вывести информацию об устройстве
     * @return
     */
    int action_info()
    {
        loadConfig();

        driver->isp_chip_info();
        close();
        return 0;
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

    void chipInfo(const QString &info);

};

#endif // PIGROAPP_H
