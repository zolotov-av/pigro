#ifndef SPI_H
#define SPI_H

#include <avrxx/io.h>
#include <avr/io.h>

namespace avr
{

    class SPI
    {
    public:

        static void init()
        {

        }

        static uint8_t read()
        {
            return avr::read(SPDR);
        }

        static void write(uint8_t value)
        {
            avr::write(SPDR, value);
        }

    };

}

#endif // SPI_H
