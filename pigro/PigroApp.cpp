#include "PigroApp.h"

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

}

PigroApp::PigroApp(const char *path, bool verbose): m_verbose(verbose), config(nano::IniReader<512>("pigro.ini").data)
{
    if ( !open(path) )
    {
        throw nano::exception(serial->errorString().toStdString());
    }
}

PigroApp::~PigroApp()
{
    if ( driver ) delete driver;
}

void PigroApp::beginProgress(int min, int max)
{
    emit beginProgress1(min, max);
}

void PigroApp::reportProgress(int value)
{
    emit reportProgress1(value);
}

void PigroApp::endProcess()
{
    emit endProgress1();
}

bool PigroApp::open(const QString &dev)
{
    serial->setPortName(dev);
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    return serial->open(QIODevice::ReadWrite);
}

void PigroApp::close()
{
    serial->close();
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
