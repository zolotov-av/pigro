#ifndef PIGRO_FIRMWARE_INFO_H
#define PIGRO_FIRMWARE_INFO_H

#include <QString>
#include <nano/config.h>


class FirmwareInfo
{
public:

    bool verbose { false };

    std::string device { };
    QString hexFileName { };
    QString hexFilePath { };
    std::string device_type { };

    nano::options projectInfo { };
    nano::options m_chip_info { };

    FirmwareInfo() = default;
    FirmwareInfo(const FirmwareInfo &) = default;
    FirmwareInfo(FirmwareInfo &&) = default;
    FirmwareInfo(const QString &path)
    {
        loadFromFile(path);
    }

    void loadFromFile(const QString &path);

    FirmwareInfo& operator = (const FirmwareInfo &) = default;
    FirmwareInfo& operator = (FirmwareInfo &&) = default;

};

#endif // PIGRO_FIRMWARE_INFO_H
