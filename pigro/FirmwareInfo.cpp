#include "FirmwareInfo.h"
#include <nano/IniReader.h>
#include <DeviceInfo.h>
#include <QDir>

void FirmwareInfo::loadFromFile(const QString &path)
{
    projectInfo = nano::IniReader<512>(path.toStdString()).data.at("main");

    if ( const std::string output = projectInfo.value("output", "quiet"); output == "verbose" )
    {
        verbose = true;
    }
    else
    {
        verbose = false;
    }

    device = projectInfo.value("device");
    if ( device.empty() )
    {
        throw nano::exception("specify device (pigro.ini)");
    }

    if ( auto dev = DeviceInfo::LoadByName(device); dev.has_value() )
    {
        m_chip_info = std::move(*dev);
    }
    else
    {
        throw nano::exception("device not found: " + device);
    }

    hexFileName = QString::fromStdString(projectInfo.value("hex"));
    if ( hexFileName.isEmpty() )
    {
        throw nano::exception("specify hex file name (pigro.ini)");
    }

    hexFilePath = QDir(path).filePath(hexFileName);

    device_type = m_chip_info.value("type", "avr");
    if ( verbose )
    {
        std::cout << "device: " << device << " (" << device_type << ")\n";
        std::cout << "device name: " << m_chip_info.value("name") << "\n";
        //std::cout << "flash_size: " << ((m_chip_info.flash_size()+1023) / 1024) << "k\n";
        std::cout << "hex_file: " << hexFilePath.toStdString() << "\n";

    }
}
