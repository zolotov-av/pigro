#ifndef PIGROPRIVATE_H
#define PIGROPRIVATE_H

#include <QObject>
#include <PigroLink.h>
#include <PigroDriver.h>
#include <AVR.h>
#include <ARM.h>
#include <FirmwareInfo.h>
#include <mutex>

class PigroApp;

class PigroPrivate: public QObject
{
    Q_OBJECT

private:

    std::mutex m_mutex { };

    PigroApp *m_app;
    PigroLink *m_link { new PigroLink(m_app, this) };

    QString m_project_path;
    FirmwareInfo m_firmware_info;

    PigroDriver* lookupDriver(const std::string &name)
    {
        if ( name == "avr" ) return new AVR(m_link);
        if ( name == "arm" ) return new ARM(m_link);
        throw nano::exception("unsupported driver: " + name);
    }

    PigroDriver* lookupDriver(const FirmwareInfo &name)
    {
        const auto driver = lookupDriver(name.device_type);
        driver->parse_device_info(name.m_chip_info);
        return driver;
    }

public slots:

    void execChipInfo(const QString tty, const QString project_path);
    void checkFirmware(const QString tty, const QString project_path);
    void writeFirmware(const QString tty, const QString project_path);

public:

    explicit PigroPrivate(PigroApp *app, QObject *parent = nullptr);
    PigroPrivate(const PigroPrivate &) = delete;
    PigroPrivate(PigroPrivate &&) = delete;

    ~PigroPrivate();

    PigroPrivate& operator = (const PigroPrivate &) = delete;
    PigroPrivate& operator = (PigroPrivate &&) = delete;

    void setProjectPath(const QString &path);

};

#endif // PIGROPRIVATE_H
