#ifndef TINY_UARTBUF_H
#define TINY_UARTBUF_H

#include <tiny/iobuf.h>
#include <tiny/system.h>

namespace tiny
{

    template <class device, uint8_t iSize = 8, uint8_t oSize = 8>
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
                device().enable_tx_empty_isr();
                return true;
            }

            return false;
        }

        void write_sync(uint8_t value)
        {
            buf.write_sync(value);
            device().enable_tx_empty_isr();
        }

        void isr_rx_ready()
        {
            buf.enque_input(device().read());
        }

        void isr_tx_empty()
        {
            uint8_t dest;
            if ( buf.deque_output(&dest) )
            {
                device().write(dest);
                return;
            }

            device().disable_tx_empty_isr();
        }

        void clear()
        {
            buf.clear();
        }

    };

}


#endif // TINY_UARTBUF_H
