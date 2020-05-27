#ifndef RINGBUF_H
#define RINGBUF_H

#include <inttypes.h>

namespace avr
{

    template <int size>
    class ringbuf
    {
    private:
        uint8_t data[size];
        uint8_t begin;
        uint8_t end;
        uint8_t len;

    public:

        bool empty() const { return len == 0; }
        bool full() const { return len == size; }

        bool read(uint8_t *dest)
        {
            if ( empty() ) return false;

            *dest = data[begin++];
            begin %= size;
            len--;
            return true;
        }

        bool write(uint8_t value)
        {
            if ( full() ) return false;

            data[end++] = value;
            end %= size;
            len++;
            return true;
        }

        void clear()
        {
            begin = 0;
            end = 0;
            len = 0;
        }

    };

}

#endif // RINGBUF_H
