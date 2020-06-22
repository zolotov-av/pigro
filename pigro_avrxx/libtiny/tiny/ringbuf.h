#ifndef TINY_RINGBUF_H
#define TINY_RINGBUF_H

#include <inttypes.h>
#include <tiny/io.h>

namespace tiny
{

    template <uint8_t size>
    class ringbuf
    {
    private:
        uint8_t data[size];
        uint8_t begin;
        uint8_t end;

    public:

        bool empty() const { return tiny::forced_read(begin) == tiny::forced_read(end); }
        bool full() const { return tiny::forced_read(begin) == (tiny::forced_read(end) + 1) % size; }

        bool read(uint8_t *dest)
        {
            const uint8_t b = tiny::forced_read(begin);
            const uint8_t e = tiny::forced_read(end);
            if ( b == e ) return false;
            *dest = tiny::forced_read(data[begin]);
            tiny::forced_write(begin, (b + 1) % size);
            return true;
        }

        bool write(uint8_t value)
        {
            const uint8_t b = tiny::forced_read(begin);
            const uint8_t e = tiny::forced_read(end);
            const uint8_t e_next = (e + 1) % size;
            if ( e_next == b ) return false;
            tiny::forced_write(data[e_next], value);
            tiny::forced_write(end, e_next);
            return true;
        }

        void clear()
        {
            tiny::forced_write(begin, 0);
            tiny::forced_write(end, 0);
        }

    };

}

#endif // TINY_RINGBUF_H
