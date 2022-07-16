#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>

#include <nano/map.h>
#include <nano/string.h>
#include <nano/exception.h>

namespace nano
{

    using options = nano::map<nano::string, nano::string>;
    using sections = nano::map<nano::string, options>;

    inline bool parse_bool(const std::string &value)
    {
        if ( value == "yes" || value == "true" ) return true;
        if ( value == "no" || value == "false" ) return false;
        throw nano::exception("wrong boolean value: " + value);
    }

    class config: public sections
    {

    public:

        config() = default;
        config(const config &) = default;
        config(config &&) = default;
        config(const sections &data): sections(data) { }
        config(sections &&data): sections(data) { }
        ~config() = default;

        config& operator = (const sections &data)
        {
            sections::operator=(data);
            return *this;
        }

        config& operator = (sections &&data)
        {
            sections::operator=(std::move(data));
            return *this;
        }

        options& section(const nano::string &name)
        {
            return operator[] (name);
        }

        string value(const string &section, const string &option, const string &default_value = { }) const
        {
            if ( auto s = this->find(section); s != this->end() )
            {
                return s->second.value(option, default_value);
            }

            return default_value;
        }

        void set(const string &section, const string &option, const string &value)
        {
            operator [] (section)[option] = value;
        }

        bool haveSection(const string &section) const
        {
            return have(section);
        }

        bool haveOption(const string &section, const string &option) const
        {
            if ( auto s = this->find(section); s != this->end() )
            {
                return s->second.have(option);
            }

            return false;
        }

        void removeSection(const string &section)
        {
            this->erase(section);
        }

        void removeOption(const string &section, const string &option)
        {
            if ( auto it = this->find(section); it != this->end() )
            {
                it->second.remove(option);
            }
        }

        void remove(const string &section)
        {
            removeSection(section);
        }

        void remove(const string &section, const string &option)
        {
            removeOption(section, option);
        }


    };

}


#endif // CONFIG_H
