#include "PigroDriver.h"

#include <nano/IniReader.h>

static bool parse_bool(const std::string &value)
{
    if ( value == "yes" || value == "true" ) return true;
    if ( value == "no" || value == "false" ) return false;
    throw nano::exception("wrong boolean value: " + value);
}

static std::optional<DeviceInfo> findInFile(const std::string &name, const std::string &path)
{
    try
    {
        nano::IniReader ini(path);
        if ( ini.haveSection(name) )
        {
            DeviceInfo avr;
            avr.type = ini.value(name, "type", "avr");
            if ( avr.type == "avr" )
            {
                // TODO: ARM
                avr.signature = PigroDriver::parseDeviceCode(ini.value(name, "device_code"));
            }
            avr.page_word_size = atoi(ini.value(name, "page_size").c_str());
            avr.page_count = atoi(ini.value(name, "page_count").c_str());
            avr.paged = parse_bool(ini.value(name, "paged", "yes"));
            return avr;
        }

        return {};
    }
    catch (const nano::exception &e)
    {
        return {};
    }
}

std::optional<DeviceInfo> PigroDriver::findDeviceByName(const std::string &name)
{
    if ( auto device = findInFile(name, "./pigro.ini"); device.has_value() )
    {
        return device;
    }

    const std::string home = getenv("HOME");

    if ( auto device = findInFile(name, home + "/.pigro/devices.ini"); device.has_value() )
    {
        return device;
    }

    if ( auto device = findInFile(name, home + "/.pigro/" + name + ".ini"); device.has_value() )
    {
        return device;
    }

    if ( auto device = findInFile(name, "/usr/share/pigro/devices.ini"); device.has_value() )
    {
        return device;
    }

    if ( auto device = findInFile(name, "/usr/share/pigro/" + name + ".ini"); device.has_value() )
    {
        return device;
    }

    return {};
}
