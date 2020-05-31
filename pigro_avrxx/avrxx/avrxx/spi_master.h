#ifndef SPI_MASTER_H
#define SPI_MASTER_H

#include <avrxx/spi.h>

namespace avr
{

    template <class spi = SPI>
    class SPI_Master
    {
    private:

        volatile bool busy;

    public:

        void wait()
        {
            while ( busy ) avr::sleep();
        }

        void ioctl(uint8_t *data, int size)
        {
            for(int i = 0; i < size; i++)
            {
                busy = true;
                spi::write(data[i]);
                wait();
                data[i] = spi::read();
            }
        }

        void handle_isr()
        {
            busy = false;
        }

    };

}

#endif // SPI_MASTER_H
