#ifndef PIGRO_JTAG_H
#define PIGRO_JTAG_H

#include <avr/io.h>
#include <avrxx/io.h>
#include <tiny/io.h>

constexpr uint8_t JTAG_DEFAULT_STATE = tiny::makebits(PA1/*JDI*/, PA3/*TMS*/ /*, PA0/ *RESET*/);

#define JD_RESET avr::pin(PORTA, PA0)
#define JTCK avr::pin(PORTA, PA2)
#define JTDI avr::pin(PORTA, PA1)
#define JTDO avr::pin(PORTA, PINA, PA5)
#define JTMS avr::pin(PORTA, PA3)
#define JRST avr::pin(PORTA, PA4)
#define JDBG avr::pin(PORTA, PA6)

class JTAG
{
protected:

    static void clk()
    {
        JTCK.set(true);
        JTCK.set(false);
    }

    static void tms(uint8_t tms)
    {
        JTMS.set(tms);
        clk();
    }

    static void init()
    {
        PORTA = JTAG_DEFAULT_STATE | (1 << PA0);
        JRST.set(1);
        JTAG::tms(0); // ->idle
    }

    static void reset()
    {
        JRST.set(0);
        JRST.set(1);
        JTAG::tms(0); // ->idle
    }

    class transaction
    {
    public:

        transaction()
        {
            JTMS.set(0);
            clk(); // capture
        }

        /**
         * Сдвигает несколько битов в shift-ir/shift-dr
         */
        static uint8_t shift_xr(uint8_t data, uint8_t bitcount)
        {
            uint8_t output = 0;
            while ( bitcount > 0 )
            {
                clk();

                JTDI.set(data & 1);
                data = data >> 1;

                output = (output >> 1) | (JTDO.value() ? 0x80 : 0);

                bitcount--;
            }
            return output;
        }

        /**
         * Сдвигает несколько битов в shift-ir/shift-dr
         */
        template <uint8_t bitcount>
        static uint8_t shift_xr(uint8_t data)
        {
            uint8_t count = bitcount;
            uint8_t output = 0;
            while ( count > 0 )
            {
                clk();

                JTDI.set(data & 1);
                data = data >> 1;

                output = (output >> 1) | (JTDO.value() ? 0x80 : 0);

                count--;
            }
            return output;
        }

        /**
         * Сдвигает несколько битов в shift-ir/shift-dr
         */
        static void shift_ex(uint8_t *data, uint8_t bitcount)
        {
            // вытолкнуть целые байты
            while ( bitcount > 8 )
            {
                *data = shift_xr<8>(*data);
                data++;
                bitcount -= 8;
            }

            // вытолкнуть остаток
            uint8_t output = shift_xr(*data, bitcount);
            for(uint8_t i = bitcount; i < 8; i++) output = output >> 1;
            *data = output;
        }

        ~transaction()
        {
            JTMS.set(1);
            clk(); // exit1
            JTDI.set(1);
            clk(); // update
        }
    };

};


#endif // PIGRO_JTAG_H
