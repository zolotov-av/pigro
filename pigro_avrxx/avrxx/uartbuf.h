#ifndef UARTBUF_H
#define UARTBUF_H

#include <avrxx.h>
#include <uart.h>
#include <iobuf.h>

namespace avr
{

    template <int iSize, int oSize, class uart = UART>
    class uartbuf
    {
    private:
        iobuf<iSize, oSize> buf;

    public:

        void wait()
        {
            while ( buf.input().empty() )
            {
                sleep();
            }
        }

        bool read(uint8_t *dest)
        {
            return buf.read(dest);
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


#endif // UARTBUF_H
