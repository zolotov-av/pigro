#ifndef NANO_INI_H
#define NANO_INI_H

#include <nano/config.h>
#include <nano/IniReader.h>

namespace nano
{

    class ini
    {
    public:

        template <int linesize = 512>
        static config loadFromFile(const string &path)
        {
            return IniReader<linesize>(path).data;
        }

    };

}

#endif // NANO_INI_H
