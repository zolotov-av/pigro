#ifndef TINY_IOBUF_H
#define TINY_IOBUF_H

#include <tiny/ringbuf.h>

namespace tiny
{

    template <int iSize, int oSize>
    class iobuf
    {
    private:

        ringbuf<iSize> ibuf;
        ringbuf<oSize> obuf;

    public:

        const auto& input() const { return ibuf; }
        const auto& output() const { return obuf; }

        bool read(uint8_t *dest)
        {
            return ibuf.read(dest);
        }

        bool write(uint8_t value)
        {
            return obuf.write(value);
        }

        void clear()
        {
            ibuf.clear();
            obuf.clear();
        }

        bool enque_input(uint8_t value)
        {
            return ibuf.write(value);
        }

        bool deque_output(uint8_t *dest)
        {
            return obuf.read(dest);
        }

    };

}

#endif // TINY_IOBUF_H
