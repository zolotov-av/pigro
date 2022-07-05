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
        try
        {
            emit m_app->chipInfo(driver->getIspChipInfo());
        }
        catch (const std::exception &e)
        {
            emit m_app->reportMessage(QStringLiteral("exception raised: %1").arg(e.what()));
        }
        catch (...)
        {
            emit m_app->reportMessage(QStringLiteral("unknown exception raised (non std::exception)"));
        }

        m_link->close();
    }

    delete driver;
}

void PigroPrivate::checkFirmware(const QString tty, const QString project_path)
{
    trace::log("PigroPrivate::checkFirmware()");

    const FirmwareInfo firmwareInfo{project_path};
    m_link->setFirmwareInfo(firmwareInfo);

    const auto driver = lookupDriver(firmwareInfo);

    m_link->setTTY(tty);
    if ( m_link->open() )
    {
        try
        {
            emit m_app->chipInfo(driver->getIspChipInfo());

            const auto pages = FirmwareData::LoadFromFile(firmwareInfo.hexFilePath.toStdString(), driver->page_size(), driver->page_fill());
            emit m_app->reportMessage(QStringLiteral("page usages: %1 / %2").arg(pages.size()).arg(driver->page_count()));
            driver->isp_check_firmware(pages);
        }
        catch (const std::exception &e)
        {
            emit m_app->reportMessage(QStringLiteral("exception raised: %1").arg(e.what()));
        }
        catch (...)
        {
            emit m_app->reportMessage(QStringLiteral("unknown exception raised (non std::exception)"));
        }

        m_link->close();
    }

    delete driver;
}

void PigroPrivate::chipErase(const QString tty, const QString project_path)
{
    trace::log("PigroPrivate::chipErase()");

    const FirmwareInfo firmwareInfo{project_path};
    m_link->setFirmwareInfo(firmwareInfo);

    const auto driver = lookupDriver(firmwareInfo);

    m_link->setTTY(tty);
    if ( m_link->open() )
    {
        try
        {
            emit m_app->chipInfo(driver->getIspChipInfo());

            //const auto pages = FirmwareData::LoadFromFile(firmwareInfo.hexFilePath.toStdString(), driver->page_size(), driver->page_fill());
            //emit m_app->reportMessage(QStringLiteral("page usages: %1 / %2").arg(pages.size()).arg(driver->page_count()));
            driver->isp_chip_erase();
        }
        catch (const std::exception &e)
        {
            emit m_app->reportMessage(QStringLiteral("exception raised: %1").arg(e.what()));
        }
        catch (...)
        {
            emit m_app->reportMessage(QStringLiteral("unknown exception raised (non std::exception)"));
        }

        m_link->close();
    }

    delete driver;
}

void PigroPrivate::writeFirmware(const QString tty, const QString project_path)
{
    trace::log("PigroPrivate::writeFirmware()");

    const FirmwareInfo firmwareInfo{project_path};
    m_link->setFirmwareInfo(firmwareInfo);

    const auto driver = lookupDriver(firmwareInfo);

    m_link->setTTY(tty);
    if ( m_link->open() )
    {
        try
        {
            emit m_app->chipInfo(driver->getIspChipInfo());

            const auto pages = FirmwareData::LoadFromFile(firmwareInfo.hexFilePath.toStdString(), driver->page_size(), driver->page_fill());
            emit m_app->reportMessage(QStringLiteral("page usages: %1 / %2").arg(pages.size()).arg(driver->page_count()));
            driver->isp_write_firmware(pages);
        }
        catch (const std::exception &e)
        {
            emit m_app->reportMessage(QStringLiteral("exception raised: %1").arg(e.what()));
        }
        catch (...)
        {
            emit m_app->reportMessage(QStringLiteral("unknown exception raised (non std::exception)"));
        }

        m_link->close();
    }

    delete driver;
}

void PigroPrivate::writeFuse(const QString tty, const QString project_path)
{
    trace::log("PigroPrivate::writeFirmware()");

    const FirmwareInfo firmwareInfo{project_path};
    m_link->setFirmwareInfo(firmwareInfo);

    const auto driver = lookupDriver(firmwareInfo);

    m_link->setTTY(tty);
    if ( m_link->open() )
    {
        try
        {
            emit m_app->chipInfo(driver->getIspChipInfo());
            driver->isp_write_fuse();
        }
        catch (const std::exception &e)
        {
            emit m_app->reportMessage(QStringLiteral("exception raised: %1").arg(e.what()));
        }
        catch (...)
        {
            emit m_app->reportMessage(QStringLiteral("unknown exception raised (non std::exception)"));
        }

        m_link->close();
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
