#ifndef PIGRO_H
#define PIGRO_H

#include <QObject>

#include "PigroLink.h"
#include "PigroDriver.h"
#include "AVR.h"
#include "ARM.h"
#include <mutex>

class Pigro: public QObject
{
    Q_OBJECT

private:

    static std::atomic<int> m_init_counter;

    bool m_verbose { false };

    std::mutex m_mutex { };

    FirmwareData m_data { };

protected:

    PigroLink *m_link { new PigroLink(this) };
    PigroDriver *driver { nullptr };

    QString m_tty;
    QString m_project_path;

    PigroDriver* lookupDriver(const std::string &name)
    {
        if ( name == "avr" ) return new AVR(m_link, this);
        if ( name == "arm" ) return new ARM(m_link, this);
        throw nano::exception("unsupported driver: " + name);
    }

    PigroDriver* lookupDriver(const FirmwareInfo &firmwareInfo)
    {
        const auto driver = lookupDriver(firmwareInfo.device_type);
        driver->setFirmwareInfo(firmwareInfo);
        driver->parse_device_info(firmwareInfo.m_chip_info);
        return driver;
    }

public:

    bool verbose() const
    {
        return m_verbose;
    }

    const QString& tty() const { return m_tty; }
    const QString& projectPath() const { return m_project_path; }

    QString protoVersion() const { return  m_link->protoVersion(); }
    uint8_t protoVersionMajor() const { return m_link->protoVersionMajor(); }
    uint8_t protoVersionMinor() const { return m_link->protoVersionMinor(); }

    FirmwareData fetchFirmwareData();

    explicit Pigro(QObject *parent = nullptr);
    Pigro(const Pigro &) = delete;
    Pigro(Pigro &&) = delete;

    ~Pigro();

    Pigro& operator = (const Pigro &) = delete;
    Pigro& operator = (Pigro &&) = delete;

    void setVerbose(bool value)
    {
        m_verbose = value;
    }

    void setTTY(const QString &device)
    {
        m_tty = device;
    }

    void setProjectPath(const QString &path)
    {
        m_project_path = path;
    }

    bool openSerialPort(const QString &tty)
    {
        setTTY(tty);
        return m_link->open(m_tty);
    }

    void openProject(const QString &path)
    {
        if ( driver ) delete driver;

        setProjectPath(path);

        FirmwareInfo firmwareInfo{ m_project_path };
        driver = lookupDriver(firmwareInfo);
        driver->setVerbose(verbose() || firmwareInfo.verbose);
    }

    void closeProject()
    {
        if ( driver )
        {
            delete driver;
            driver = nullptr;
        }
    }

    void closeSerialPort()
    {
        closeProject();
        m_link->close();
    }

    void close()
    {
        if ( driver )
        {
            delete driver;
            driver = nullptr;
        }

        m_link->close();
    }

    FirmwareData readHEX()
    {
        auto pages = FirmwareData::LoadFromFile(driver->firmwareInfo().hexFilePath.toStdString(), driver->page_size(), driver->page_fill());
        printf("page usages: %ld / %d\n", pages.size(), driver->page_count());
        return pages;
    }

    /**
     * Прочитать информацию о чипе
     */
    QString getChipInfo()
    {
        return driver->getIspChipInfo();
    }

public slots:

    /**
     * Действие - вывести информацию об устройстве
     */
    void action_info();

    /**
     * Действие - проверить прошивку и вывести статистику (без обращения к чипу)
     */
    void action_stat();

    /**
     * Действие - проверить прошивку в устройстве
     */
    void action_check();

    /**
     * Действие - записать прошивку в устройство
     */
    void action_write();

    /**
     * Действие - стереть чип
     */
    void action_erase();

    /**
     * Действие - прочитать биты fuse
     */
    void action_read_fuse();

    /**
     * Действие - записать биты биты fuse в устройство
     */
    void action_write_fuse();

    /**
     * test STM32/JTAG
     */
    void action_test();

    void isp_chip_info(const QString tty, const QString project_path);
    void isp_check_firmware(const QString tty, const QString project_path);
    void isp_write_firmware(const QString tty, const QString project_path);
    void isp_chip_erase(const QString tty, const QString project_path);
    void isp_write_fuse(const QString tty, const QString project_path);

    /**
     * Прочитать прошивку зашитую в устройство
     */
    void readFirmware(const QString tty, const QString project_path);

signals:

    void sessionStarted(int major, int minor);
    void sessionStopped();

    void beginProgress(int min, int max);
    void reportProgress(int value);
    void reportMessage(const QString &message);
    void endProgress();

    void chipInfo(const QString &info);
    void dataReady();

};

#endif // PIGRO_H
