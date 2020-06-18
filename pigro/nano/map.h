#ifndef NANO_MAP_H
#define NANO_MAP_H

#include <map>

namespace nano
{

    template <class key_t, class value_t>
    class map: public std::map<key_t, value_t>
    {
    public:

        map() = default;
        map(const map &other) = default;
        map(map &&other) = default;
        map(const std::map<key_t, value_t> &other): std::map<key_t, value_t>(other) { }
        ~map() = default;

        map& operator = (const map &other) = default;
        map& operator = (map && other) = default;

        bool have(const key_t &name) const
        {
            return this->find(name) != this->end();
        }

        value_t value(const key_t &name, const value_t &default_value = {}) const
        {
            if ( auto it = this->find(name); it != this->end() )
            {
                return it->second;
            }

            return default_value;
        }

        void set(const key_t &name, const value_t &value)
        {
            std::map<key_t, value_t>::operator[](name) = value;
        }

        void remove(const key_t &name)
        {
            this->erase(name);
        }

    };

}

#endif // NANO_MAP_H
