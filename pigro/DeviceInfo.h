#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <array>
#include <optional>
#include <nano/config.h>
#include <nano/ini.h>
#include <nano/math.h>

struct DeviceInfo
{

    /**
     * Загрузить описание чипа из файла
     */
    static std::optional<nano::options> LoadFromFile(const std::string &name, const std::string &path)
    {
        try
        {
            nano::config ini = nano::ini::loadFromFile(path);
            if ( ini.haveSection(name) )
            {
                return ini.section(name);
            }

            return {};
        }
        catch (const nano::exception &e)
        {
            return {};
        }
    }

    /**
     * Найти описание чипа по имени
     */
    static std::optional<nano::options> LoadByName(const std::string &name)
    {
        if ( auto device = LoadFromFile(name, "pigro.ini"); device.has_value() )
        {
            return device;
        }

        const std::string home = getenv("HOME");

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

        return {};
    }
};


#endif // DEVICEINFO_H
