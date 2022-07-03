#include "PigroPrivate.h"
#include <PigroApp.h>
#include "trace.h"

void PigroPrivate::execChipInfo(const QString tty, const QString project_path)
{
    trace::log("PigroPrivate::execChipInfo()");

    const FirmwareInfo firmwareInfo{project_path};
    m_link->setFirmwareInfo(firmwareInfo);

    const auto driver = lookupDriver(firmwareInfo);

    m_link->setTTY(tty);
    if ( m_link->open() )
    {
        const auto info = driver->getIspChipInfo();
        m_link->close();
        emit m_app->chipInfo(info);
    }

    delete driver;
}

PigroPrivate::PigroPrivate(PigroApp *app, QObject *parent): QObject(parent), m_app(app)
{
    trace::log("PigroPrivate create");
}

PigroPrivate::~PigroPrivate()
{
    trace::log("PigroPrivate destroy");
}

void PigroPrivate::setProjectPath(const QString &path)
{
    trace::log(QStringLiteral("PigroPrivate::setProjectPath(%1)").arg(path));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_project_path = path;
}
