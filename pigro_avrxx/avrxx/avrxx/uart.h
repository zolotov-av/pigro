#ifndef UART_H
#define UART_H

#include <avrxx/io.h>
#include <avr/io.h>

namespace avr
{

    class UART
    {
    public:

        static void enableRXC()
        {
            avr::ioreg(UCSRB).setPin(RXCIE, true);
        }

        static void disableRXC()
        {
            avr::ioreg(UCSRB).setPin(RXCIE, false);
        }

        static void enableTXC()
        {
            avr::ioreg(UCSRB).setPin(TXCIE, true);
        }

        static void disableTXC()
        {
            avr::ioreg(UCSRB).setPin(TXCIE, false);
        }

        static void enableUDRE()
        {
            avr::ioreg(UCSRB).write_pin(UDRIE, true);
        }

        static void disableUDRE()
        {
            avr::ioreg(UCSRB).write_pin(UDRIE, false);
        }

        static void enableTransmitter()
        {
            avr::ioreg(UCSRB).setPin(TXEN, true);
        }

        static void disableTransmitter()
        {
            avr::ioreg(UCSRB).setPin(TXEN, false);
        }

        static void enableReceiver()
        {
            avr::ioreg(UCSRB).setPin(RXEN, true);
        }

        static void disableReceiver()
        {
            avr::ioreg(UCSRB).setPin(RXEN, false);
        }

        static void setBaudRate(int rate)
        {
            avr::ioreg(UBRRH).raw_write( (rate / 256) & 0xF );
            avr::ioreg(UBRRL).raw_write(rate % 256);
        }

        static uint8_t read()
        {
            return avr::ioreg(UDR).read();
        }

        static void write(uint8_t value)
        {
            avr::ioreg(UDR).write(value);
        }

    };

    class UARTConfig
    {
    public:
        uint8_t regC = makebits(URSEL, UCSZ1, UCSZ0);

        void enableSyncMode()
        {
            avr::ioreg(regC).setPin(UMSEL, false);
        }

        void disableSyncMode()
        {
            avr::ioreg(regC).setPin(UMSEL, true);
        }

        void getUPM()
        {
            avr::bitfield(regC, UPM0, 2).value();
        }

        void setUPM(uint8_t value)
        {
            avr::bitfield(regC, UPM0, 2).set(value);
        }

        uint8_t characterSize() const
        {
            const uint8_t lo = avr::bitfield(regC, UCSZ0, 2).value();
            const uint8_t hi = avr::bitfield(UCSRB, UCSZ2, 1).value();
            const uint8_t value = lo + (hi << 2);
            switch ( value )
            {
            case 0: return 5;
            case 1: return 6;
            case 2: return 7;
            case 3: return 8;
            case 7: return 9;
            default: return 0;
            }
        }

        uint8_t encodeCharacterSize(uint8_t size)
        {
            switch ( size )
            {
            case 5: return 0;
            case 6: return 1;
            case 7: return 2;
            case 8: return 3;
            case 9: return 7;
            default: return 0;
            }
        }

        void setCharacterSize(uint8_t size)
        {
            uint8_t value = encodeCharacterSize(size);
            avr::bitfield(regC, UCSZ0, 2).set(value);
            avr::bitfield(UCSRB, UCSZ2, 1).set(value >> 2);
        }

        void apply()
        {
            avr::ioreg(UCSRC).write(regC);
        }

    };

}

#endif // UART_H
