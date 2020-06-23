#ifndef PIGRO_JTAG_H
#define PIGRO_JTAG_H

#include <avr/io.h>
#include <avrxx/io.h>
#include <tiny/io.h>

constexpr uint8_t JTAG_DEFAULT_STATE = tiny::makebits(PA1/*JDI*/, PA3/*TMS*/ /*, PA0/ *RESET*/);

#define JTCK avr::pin(PORTA, PA2)
#define JTDI avr::pin(PORTA, PA1)
#define JTDO avr::pin(PORTA, PINA, PA5)
#define JTMS avr::pin(PORTA, PA3)
#define JRST avr::pin(PORTA, PA4)
#define JDBG avr::pin(PORTA, PA6)

class JTAG
{
public:

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

    static void reset(uint8_t data)
    {
        switch (data)
        {
        case 0:
            PORTA = JTAG_DEFAULT_STATE | (1 << PA0);
            JRST.set(1);
            return;
        case 1:
            avr::pin(PORTA, PA0).set(1);
            return;
        case 2:
            avr::pin(PORTA, PA0).set(0);
            return;
        }
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
