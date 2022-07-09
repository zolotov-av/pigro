#ifndef PIGRO_INI_READER_H
#define PIGRO_INI_READER_H

#include <QString>
#include <nano/string.h>
#include <nano/config.h>

#include <map>

namespace nano
{

    class IniReader
    {
    public:

        sections data;

        IniReader() = default;
        IniReader(const IniReader &) = delete;
        IniReader(IniReader &&) = default;
        //IniReader(const std::string &path) { open(QString::fromStdString(path)); }
        IniReader(const QString &path) { open(path); }

        ~IniReader() = default;

        void open(const QString &path);

    };

}

#endif // PIGRO_INI_READER_H
