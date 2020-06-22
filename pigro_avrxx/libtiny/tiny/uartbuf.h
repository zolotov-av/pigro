#ifndef TINY_UARTBUF_H
#define TINY_UARTBUF_H

#include <tiny/iobuf.h>
#include <tiny/system.h>

namespace tiny
{

    template <uint8_t iSize, uint8_t oSize, class uart>
    class uartbuf
    {
    private:

        iobuf<iSize, oSize> buf;

    public:

        bool read(uint8_t *dest)
        {
            return buf.read(dest);
        }

        void read_sync(uint8_t *dest)
        {
            return buf.read_sync(dest);
        }

        bool write(uint8_t value)
        {
            if ( buf.write(value) )
            {
                uart::enableUDRE();
                return true;
            }

            return false;
        }

        void write_sync(uint8_t value)
        {
            buf.write_sync(value);
            uart::enableUDRE();
        }

        void handle_rxc_isr()
        {
            buf.enque_input(uart::read());
        }

        void handle_txc_isr()
        {
        }

        void handle_udre_isr()
        {
            uint8_t dest;
            if ( buf.deque_output(&dest) )
            {
                uart::write(dest);
                if ( buf.output().empty() )
                {
                    uart::disableUDRE();
                }
            }

        }

        void clear()
        {
            buf.clear();
        }

    };

}


#endif // TINY_UARTBUF_H
