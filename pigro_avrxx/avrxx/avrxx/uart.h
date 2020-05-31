#ifndef UART_H
#define UART_H

#include <avrxx/io.h>
#include <avr/io.h>

namespace avr
{

    class UART
    {
    public:

        static void init()
        {
            UCSRA = makebits();
            UCSRB = makebits(RXEN, TXEN, RXCIE, TXCIE);
            UCSRC = makebits(URSEL, UCSZ0, UCSZ1);
        }

        static void enableUDRE()
        {
            avr::ioreg(UCSRB).write_pin(UDRIE, true);
        }

        static void disableUDRE()
        {
            avr::ioreg(UCSRB).write_pin(UDRIE, false);
        }

        static void setBaudRate(int rate)
        {
            avr::ioreg(UBRRH).raw_write( (rate / 256) & 0xF );
            avr::ioreg(UBRRL).raw_write(rate % 256);
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
