#ifndef NANO_INI_H
#define NANO_INI_H

#include <nano/config.h>
#include <nano/IniReader.h>

namespace nano
{

    class ini
    {
    public:

        static config loadFromFile(const string &path)
        {
            return IniReader(QString::fromStdString(path)).data;
        }

    };

}

#endif // NANO_INI_H
