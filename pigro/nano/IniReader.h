#ifndef INIREADER_H
#define INIREADER_H

#include <nano/string.h>
#include <nano/TextReader.h>

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
    private:

        using options = std::map<std::string, std::string>;
        using sections = std::map<std::string, options>;

        sections data;

    public:

        IniReader() = default;
        IniReader(const IniReader &) = delete;
        IniReader(IniReader &&) = default;
        IniReader(const char *path) { open(path); }
        ~IniReader() = default;

        void open(const char *path)
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
                //std::cout << "key: " << quote(key) << " value: " << quote(value) << std::endl;

                data[current_section][std::string(key)] = std::string(value);
            }

            std::cout << "sections:\n";
            for(const auto &t : data)
            {
                std::cout << t.first << "\n";
            }
            std::cout << std::flush;

        }

        bool haveSection(const std::string &section) const
        {
            return data.find(section) != data.end();
        }

        bool haveOption(const std::string &section, const std::string &option) const
        {
            if ( auto s = data.find(section); s != data.end() )
            {
                const auto &options = s->second;
                return options.find(option) != options.end();
            }

            return false;
        }

        std::string value(const std::string &section, const std::string &option, std::string_view defaultValue = {}) const
        {
            if ( auto s = data.find(section); s != data.end() )
            {
                const auto &options = s->second;
                if ( auto opt = options.find(option); opt != options.end() )
                {
                    return opt->second;
                }
                else
                {
                    std::cout << "option not found: " << option << std::endl;
                }
            }
            else
            {
                std::cout << "section not found: " << section << std::endl;
            }

            return std::string(defaultValue);
        }

        void setValue(const std::string &section, const std::string &option, const std::string &value)
        {
            data[section][option] = value;
        }

    };

}

#endif // INIREADER_H
