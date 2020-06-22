#ifndef UART_H
#define UART_H

#include <avrxx/io.h>
#include <avr/io.h>
#include <tiny/io.h>

namespace avr
{

    class UART
    {
    public:

        static void init()
        {
            UCSRA = tiny::makebits();
            UCSRB = tiny::makebits(RXEN, TXEN, RXCIE);
            UCSRC = tiny::makebits(URSEL, UCSZ0, UCSZ1);
        }

        static void setBaudRate(int rate)
        {
            avr::ioreg(UBRRH).raw_write( (rate / 256) & 0xF );
            avr::ioreg(UBRRL).raw_write(rate % 256);
        }

        static void enable_tx_empty_isr()
        {
            avr::ioreg(UCSRB).write_pin(UDRIE, true);
        }

        static void disable_tx_empty_isr()
        {
            avr::ioreg(UCSRB).write_pin(UDRIE, false);
        }

        static uint8_t read()
        {
            return avr::read(UDR);
        }

        static void write(uint8_t value)
        {
            avr::write(UDR, value);
        }

    };

}

#endif // UART_H
