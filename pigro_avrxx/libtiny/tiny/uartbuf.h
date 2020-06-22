#ifndef TINY_UARTBUF_H
#define TINY_UARTBUF_H

#include <tiny/iobuf.h>
#include <tiny/system.h>

namespace tiny
{

    class no_Lock
    {

    };

    template <int iSize, int oSize, class uart, class ilock = tiny::interrupt_lock>
    class uartbuf
    {
    private:

        iobuf<iSize, oSize> buf;

    public:

        void wait()
        {
            while ( buf.input().empty() )
            {
                tiny::sleep();
            }
        }

        bool read(uint8_t *dest)
        {
            ilock lock;
            return buf.read(dest);
        }

        bool read_sync(uint8_t *dest)
        {
            wait();
            return read(dest);
        }

        bool write(uint8_t value)
        {
            ilock lock;
            if ( buf.write(value) )
            {
                uart::enableUDRE();
                return true;
            }

            return false;
        }

        void write_sync(uint8_t value)
        {
            while ( buf.output().full() )
            {
                sleep();
            }

            write(value);
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
