#ifndef UARTBUF_H
#define UARTBUF_H

#include <avr/io.h>
#include <avrxx/io.h>
#include <avrxx/uart.h>
#include <avrxx/iobuf.h>

namespace avr
{

    class intr_lock
    {
    public:

        intr_lock()
        {
            avr::interrupt_disable();
        }

        ~intr_lock()
        {
            avr::interrupt_enable();
        }
    };

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
            intr_lock lock;
            return buf.read(dest);
        }

        bool read_sync(uint8_t *dest)
        {
            wait();
            return read(dest);
        }

        bool write(uint8_t value)
        {
            intr_lock lock;
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

#endif // UARTBUF_H
