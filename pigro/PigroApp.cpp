#include "PigroApp.h"
#include "trace.h"
#include <QDebug>

std::atomic<int> PigroApp::m_init_counter { 0 };

void PigroApp::threadStarted()
{
    trace::setThreadName("pigro");
    trace::log("PigroApp::threadStarted()");
    m_private = new PigroPrivate();
    emit started();
}

void PigroApp::threadFinished()
{
    trace::log("PigroApp::threadFinished()");
    delete m_private;
    m_private = nullptr;
    emit stopped();
}

QString PigroApp::protoVersion() const
{
    if ( nack_support )
    {
        return QStringLiteral("%1.%2 (NACK support)").arg(protoVersionMajor()).arg(protoVersionMinor());
    }
    else
    {
        return QStringLiteral("%1.%2").arg(protoVersionMajor()).arg(protoVersionMinor());
    }
}

PigroApp::PigroApp(QObject *parent): QObject(parent)
{
    if ( ++m_init_counter == 1 )
    {
        qDebug() << "Q_INIT_RESOURCE(PigroResources)";
        Q_INIT_RESOURCE(PigroResources);
    }

    connect(m_thread, &QThread::started, this, &PigroApp::threadStarted, Qt::DirectConnection);
    connect(m_thread, &QThread::finished, this, &PigroApp::threadFinished, Qt::DirectConnection);
}

PigroApp::~PigroApp()
{
    if ( driver ) delete driver;
    if ( --m_init_counter == 0 )
    {
        qDebug() << "Q_CLEANUP_RESOURCE(PigroResources)";
        Q_CLEANUP_RESOURCE(PigroResources);
    }
}

void PigroApp::open(const char *ttyPath, const char *configPath)
{
    config = {nano::IniReader<512>(configPath).data};

    if ( !open(ttyPath) )
    {
        throw nano::exception(link->errorString().toStdString());
    }
}

void PigroApp::loadConfig()
{
    if ( const std::string output = config.value("main", "output", "quiet"); output == "verbose" )
    {
        m_verbose = true;
    }

    std::string device = config.value("main", "device");
    if ( device.empty() )
    {
        throw nano::exception("specify device (pigro.ini)");
    }

    hexfname = config.value("main", "hex");
    if ( hexfname.empty() )
    {
        throw nano::exception("specify hex file name (pigro.ini)");
    }

    if ( auto dev = DeviceInfo::LoadByName(device); dev.has_value() )
    {
        m_chip_info = std::move(*dev);
    }
    else
    {
        throw nano::exception("device not found: " + device);
    }

    device_type = m_chip_info.value("type", "avr");
    if ( verbose() )
    {
        std::cout << "device: " << device << " (" << device_type << ")\n";
        std::cout << "device name: " << m_chip_info.value("name") << "\n";
        //std::cout << "flash_size: " << ((m_chip_info.flash_size()+1023) / 1024) << "k\n";
        std::cout << "hex_file: " << hexfname << "\n";

    }

    if ( driver ) delete driver;
    driver = lookupDriver(device_type);
    driver->parse_device_info(m_chip_info);
}

void PigroApp::loadConfig(const QString &path)
{
    config = nano::IniReader<512>(path.toStdString()).data;
    loadConfig();
}

