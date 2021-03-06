#include "Pigro.h"
#include "trace.h"

std::atomic<int> Pigro::m_init_counter { 0 };


FirmwareData Pigro::fetchFirmwareData()
{
    const std::lock_guard lock(m_mutex);

    return std::move(m_data);
}

Pigro::Pigro(QObject *parent): QObject(parent)
{
    if ( ++m_init_counter == 1 )
    {
        Q_INIT_RESOURCE(PigroResources);
    }

    connect(m_link, &PigroLink::errorOccurred, this, &Pigro::reportMessage, Qt::DirectConnection);
    connect(m_link, &PigroLink::sessionStarted, this, &Pigro::sessionStarted, Qt::DirectConnection);
    connect(m_link, &PigroLink::sessionStopped, this, &Pigro::sessionStopped, Qt::DirectConnection);
}

Pigro::~Pigro()
{
    if ( driver )
    {
        delete driver;
    }

    if ( --m_init_counter == 0 )
    {
        Q_CLEANUP_RESOURCE(PigroResources);
    }
}

void Pigro::action_info()
{
    driver->isp_chip_info();
}

void Pigro::action_stat()
{
    const FirmwareData pages = readHEX();
    driver->isp_stat_firmware(pages);
}

void Pigro::action_check()
{
    const FirmwareData pages = readHEX();
    driver->isp_check_firmware(pages);
}

void Pigro::action_write()
{
    FirmwareData pages = readHEX();
    driver->isp_write_firmware(pages);
}

void Pigro::action_erase()
{
    driver->isp_chip_erase();
}

void Pigro::action_read_fuse()
{
    driver->isp_read_fuse();
}

void Pigro::action_write_fuse()
{
    driver->isp_write_fuse();
}

void Pigro::action_test()
{
    driver->action_test();
}

void Pigro::isp_chip_info(const QString tty, const QString project_path)
{
    emit reportMessage("Pigro::isp_chip_info()");

    const FirmwareInfo firmwareInfo{project_path};
    driver = lookupDriver(firmwareInfo);

    emit beginProgress(0, 100);

    if ( m_link->open(tty) )
    {
        try
        {
            const QString info = driver->getIspChipInfo();
            emit chipInfo(info);
            emit reportResult(info);
        }
        catch (const std::exception &e)
        {
            emit reportException(e.what());
        }
        catch (...)
        {
            emit reportException(QStringLiteral("unknown exception"));
        }

        m_link->close();
    }

    delete driver;
    driver = nullptr;

    emit endProgress();
}

void Pigro::isp_check_firmware(const QString tty, const QString project_path)
{
    emit reportMessage("Pigro::isp_check_firmware()");

    const FirmwareInfo firmwareInfo{project_path};
    driver = lookupDriver(firmwareInfo);

    if ( m_link->open(tty) )
    {
        try
        {
            emit chipInfo(driver->getIspChipInfo());

            const auto pages = FirmwareData::LoadFromFile(firmwareInfo.hexFilePath.toStdString(), driver->page_size(), driver->page_fill());
            emit reportMessage(QStringLiteral("page usages: %1 / %2").arg(pages.size()).arg(driver->page_count()));
            driver->isp_check_firmware(pages);
        }
        catch (const std::exception &e)
        {
            emit reportException(e.what());
        }
        catch (...)
        {
            emit reportException(QStringLiteral("unknown exception"));
        }

        m_link->close();
    }

    emit endProgress();
    delete driver;
    driver = nullptr;
}

void Pigro::isp_write_firmware(const QString tty, const QString project_path)
{
    trace::log("PigroPrivate::writeFirmware()");

    const FirmwareInfo firmwareInfo{project_path};
    driver = lookupDriver(firmwareInfo);

    if ( m_link->open(tty) )
    {
        emit sessionStarted(m_link->protoVersionMajor(), m_link->protoVersionMinor());
        try
        {
            emit chipInfo(driver->getIspChipInfo());

            const auto pages = FirmwareData::LoadFromFile(firmwareInfo.hexFilePath.toStdString(), driver->page_size(), driver->page_fill());
            emit reportMessage(QStringLiteral("page usages: %1 / %2").arg(pages.size()).arg(driver->page_count()));
            driver->isp_write_firmware(pages);
        }
        catch (const std::exception &e)
        {
            emit reportException(e.what());
        }
        catch (...)
        {
            emit reportException(QStringLiteral("unknown exception"));
        }

        m_link->close();
    }

    delete driver;
    driver = nullptr;
}

void Pigro::isp_chip_erase(const QString tty, const QString project_path)
{
    trace::log("PigroPrivate::chipErase()");

    const FirmwareInfo firmwareInfo{project_path};
    driver = lookupDriver(firmwareInfo);

    if ( m_link->open(tty) )
    {
        try
        {
            emit chipInfo(driver->getIspChipInfo());

            //const auto pages = FirmwareData::LoadFromFile(firmwareInfo.hexFilePath.toStdString(), driver->page_size(), driver->page_fill());
            //emit m_app->reportMessage(QStringLiteral("page usages: %1 / %2").arg(pages.size()).arg(driver->page_count()));
            driver->isp_chip_erase();
        }
        catch (const std::exception &e)
        {
            emit reportException(e.what());
        }
        catch (...)
        {
            emit reportException(QStringLiteral("unknown exception"));
        }

        m_link->close();
    }

    emit endProgress();

    delete driver;
    driver = nullptr;
}

void Pigro::isp_write_fuse(const QString tty, const QString project_path)
{
    trace::log("PigroPrivate::writeFirmware()");

    const FirmwareInfo firmwareInfo{project_path};
    const auto driver = lookupDriver(firmwareInfo);

    if ( m_link->open(tty) )
    {
        try
        {
            emit chipInfo(driver->getIspChipInfo());
            driver->isp_write_fuse();
        }
        catch (const std::exception &e)
        {
            emit reportException(e.what());
        }
        catch (...)
        {
            emit reportException(QStringLiteral("unknown exception"));
        }

        m_link->close();
    }

    delete driver;
}

void Pigro::readFirmware(const QString tty, const QString project_path)
{
    const FirmwareInfo firmwareInfo{project_path};
    driver = lookupDriver(firmwareInfo);

    if ( m_link->open(tty) )
    {
        try
        {
            emit chipInfo(driver->getIspChipInfo());
            const auto data = driver->readFirmware();

            const std::lock_guard lock(m_mutex);
            m_data = data;
        }
        catch (const std::exception &e)
        {
            emit reportException(e.what());
        }
        catch (...)
        {
            emit reportException(QStringLiteral("unknown exception"));
        }

        m_link->close();
    }

    delete driver;
    driver = nullptr;

    emit dataReady();
}

void Pigro::cancel()
{
    emit reportMessage("Pigro::cancel()");
    if ( driver )
    {
        emit reportMessage("Pigro::cancel() driver->cancel");
        driver->cancel();
    }
}
