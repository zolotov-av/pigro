#ifndef INIREADER_H
#define INIREADER_H

#include <nano/string.h>
#include <nano/TextReader.h>
#include <nano/config.h>

#include <map>

namespace nano
{

    inline std::string quote(std::string_view text)
    {
        return "\"" + std::string(text) + "\"";
    }

    template <int linesize = 512>
    class IniReader
    {
    public:

        sections data;

        IniReader() = default;
        IniReader(const IniReader &) = delete;
        IniReader(IniReader &&) = default;
        IniReader(const std::string &path) { open(path); }
        ~IniReader() = default;

        void open(const std::string &path)
        {
            TextReader<linesize> reader(path);

            std::string current_section {};

            while ( !reader.eof() )
            {
                std::string line = reader.readLine();

                const std::string_view sv = nano::trim(line);
                if ( sv.empty() ) continue;
                if ( sv[0] == '#' ) continue;
                if ( sv[0] == '[' )
                {
                    if ( sv.back() != ']' )
                    {
                        throw exception(std::string("wrong ini syntax (section): ") + path);
                    }

                    current_section = nano::trim(sv.substr(1, sv.length()-2));
                    //std::cout << "new section: " << quote(current_section) << std::endl;
                    continue;
                }

                const auto pos = sv.find('=');

                if ( pos == sv.npos ) throw exception(std::string("wrong ini syntax (value): ") + path);
                const auto key = nano::trim(sv.substr(0, pos));
                const auto value = nano::trim(sv.substr(pos+1, sv.length()-pos-1));

                data[current_section][std::string(key)] = std::string(value);
            }

        }

    };

}

#endif // INIREADER_H
