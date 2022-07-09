#ifndef NANO_INI_H
#define NANO_INI_H

#include <nano/config.h>
#include <nano/IniReader.h>

namespace nano
{

    class ini
    {
    public:

        static config loadFromFile(const std::string &path)
        {
            return IniReader(QString::fromStdString(path)).data;
        }

        static config loadFromFile(const QString &path)
        {
            return IniReader(path).data;
        }

    };

}

#endif // NANO_INI_H
