#ifndef SPIBUF_H
#define SPIBUF_H

#include <iobuf.h>
#include <avr/io.h>

namespace avr
{

    template <int iSize, int oSize>
    class spibuf
    {
    private:

        iobuf<iSize, oSize> buf;
        bool out_ready;

    public:

        bool read(uint8_t *dest)
        {
            return buf.read(dest);
        }

        bool write(uint8_t value)
        {
            return buf.write(value);
        }

        void handle_isr()
        {
            char data1 = SPDR;

            uint8_t dest;


            if ( buf.deque_output(&dest) )
            {
                SPDR = dest;
            }
            else
            {
                out_ready = true;
            }

            buf.enque_input(data1);
        }

        void clear()
        {
            buf.clear();
        }
    };

}


#endif // SPIBUF_H
