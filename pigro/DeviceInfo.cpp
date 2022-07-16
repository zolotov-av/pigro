#include "DeviceInfo.h"

#include <QDebug>

std::optional<nano::options> DeviceInfo::LoadFromFile(const QString &n, const QString &path)
{
    try
    {
        const std::string name = n.toStdString();
        nano::config ini = nano::ini::loadFromFile(path);
        if ( ini.haveSection(name) )
        {
            qDebug() << QStringLiteral("DeviceInfo::LoadFromFile(%1) loaded from %2").arg(n, path);
            return ini.section(name);
        }

        return {};
    }
    catch (const nano::exception &e)
    {
        return {};
    }
}

std::optional<nano::options> DeviceInfo::LoadByName(const QString &name)
{

    if ( auto device = LoadFromFile(name, "pigro.ini"); device.has_value() )
    {
        return device;
    }

    const QString home = QDir::homePath();

    if ( auto device = LoadFromFile(name, home + "/.pigro/devices.ini"); device.has_value() )
    {
        return device;
    }

    if ( auto device = LoadFromFile(name, home + "/.pigro/" + name + ".ini"); device.has_value() )
    {
        return device;
    }

    if ( auto device = LoadFromFile(name, "/usr/share/pigro/devices.ini"); device.has_value() )
    {
        return device;
    }

    if ( auto device = LoadFromFile(name, "/usr/share/pigro/" + name + ".ini"); device.has_value() )
    {
        return device;
    }

    if ( auto device = LoadFromFile(name, ":/data/avrdude.ini"); device.has_value() )
    {
        return device;
    }

    if ( auto device = LoadFromFile(name, ":/data/stm32.ini"); device.has_value() )
    {
        return device;
    }

    return {};
}
