#ifndef TINY_JTAG_H
#define TINY_JTAG_H

#include <avr/io.h>
#include <avrxx/io.h>

constexpr uint8_t JTAG_DEFAULT_STATE = avr::makebits(PA0/*RESET*/, PA1/*JDI*/, PA3/*TMS*/);

#define JTCK avr::pin(PORTA, PA2)
#define JTDI avr::pin(PORTA, PA1)
#define JTDO avr::pin(PORTA, PINA, PA5)
#define JTMS avr::pin(PORTA, PA3)
#define JRST avr::pin(PORTA, PA4)
#define JDBG avr::pin(PORTA, PA6)

namespace tiny
{

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

        static void reset()
        {
            PORTA = JTAG_DEFAULT_STATE;
            JRST.set(1);
        }

        /**
         * включает JTAG
         * на выходе TMS=1, JRST=1, JDI=1
         */
        static void begin()
        {
            reset();
        }

        /**
         * выключает JTAG
         * на выходе TMS=1, JRST=0, JDI=1
         */
        static void end()
        {
            PORTA = JTAG_DEFAULT_STATE;
        }

        /**
         * на входе TMS=1
         * на выходе TMS=1, 2-clk
         */
        static void shift(uint8_t *data, uint8_t bitcount)
        {
            JTMS.set(0);
            clk(); // capture

            uint8_t dr = *data;
            uint8_t bit = 0;
            uint8_t output = 0;
            for(uint8_t i = 0; i < bitcount; i++)
            {
                clk();

                JTDI.set(dr & 1);
                dr = dr >> 1;

                output = (output >> 1) | (JTDO.value() ? 0x80 : 0);

                bit ++;
                if ( bit == 8 )
                {
                    *data++ = output;
                    dr = *data;
                    bit = 0;
                    output = 0;
                }
            }

            if ( bit != 0 )
            {
                for(uint8_t i = bit; i < 8; i++) output = output >> 1;
                *data = output;
            }

            JTMS.set(1);
            clk(); // exit1
            JTDI.set(1); // default state
            clk(); // update
        }

        /**
         * на входе TMS=0 (idle)
         * на выходе TMS=1, 2-clk
         */
        static uint8_t set_ir(uint8_t ir)
        {
            JTMS.set(1);
            clk(); // ->select-dr
            clk(); // ->select-ir
            uint8_t buf[2];
            buf[0] = ir | 0xF0;
            buf[1] = 0xFF;
            shift(buf, 9); // TMS=1, 2-clk
            return buf[0];
        }

        /**
         * на входе TMS=0 (idle) или TMS=1, 2-clk
         * на выходе TMS=1, 2-clk
         */
        static void set_dr(uint8_t *data, uint8_t bitcount)
        {
            JTMS.set(1);
            clk(); // ->select-dr
            shift(data, bitcount+1); // TMS=1, 2-clk
        }

    };

}

#endif // TINY_JTAG_H
