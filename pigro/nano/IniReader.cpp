#include "IniReader.h"

#include <QFile>
#include <QTextStream>

namespace nano
{

    void IniReader::open(const QString &path)
    {
        QFile f(path);
        if ( !f.open(QIODevice::ReadOnly | QIODevice::Text) )
        {
            throw exception(QStringLiteral("fail to open requested file: ").append(path));
        }

        QTextStream reader(&f);

        std::string current_section {};

        while ( !reader.atEnd() )
        {
            const std::string line = reader.readLine().toStdString();

            const std::string_view sv = nano::trim(line);
            if ( sv.empty() ) continue;
            if ( sv[0] == '#' ) continue;
            if ( sv[0] == '[' )
            {
                if ( sv.back() != ']' )
                {
                    throw exception(QStringLiteral("wrong ini syntax (section): ").append(path));
                }

                current_section = nano::trim(sv.substr(1, sv.length()-2));
                //std::cout << "new section: " << quote(current_section) << std::endl;
                continue;
            }

            const auto pos = sv.find('=');

            if ( pos == sv.npos ) throw exception(QStringLiteral("wrong ini syntax (value): ").append(path));
            const auto key = nano::trim(sv.substr(0, pos));
            const auto value = nano::trim(sv.substr(pos+1, sv.length()-pos-1));

            data[current_section][std::string(key)] = std::string(value);
        }

    }

}
