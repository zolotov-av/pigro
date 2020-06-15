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
                JTDI.set(1); // default state
                clk(); // update
            }
        };

        ///////////////////////////////////////////////////////////////////
        /// STM32 Debug Port

        /**
         * на входе TMS=0 (idle)
         * на выходе TMS=1, 2-clk
         */
        static uint8_t set_ir(uint8_t ir)
        {
            JTMS.set(1);
            clk(); // ->select-dr
            clk(); // ->select-ir

            transaction t;

            uint8_t output = t.shift_xr<4>(ir);
            t.shift_xr<5>(0xFF);

            return output;
        }

        /**
         * на входе TMS=0 (idle) или TMS=1, 2-clk
         * на выходе TMS=1, 2-clk
         */
        static void set_dr(uint8_t *data, uint8_t bitcount)
        {
            JTMS.set(1);
            clk(); // ->select-dr

            transaction t;

            // вытолкнуть целые байты
            while ( bitcount > 8 )
            {
                *data = t.shift_xr<8>(*data);
                data++;
                bitcount -= 8;
            }

            // вытолкнуть остаток
            uint8_t output = t.shift_xr(*data, bitcount);
            for(uint8_t i = bitcount; i < 8; i++) output = output >> 1;
            *data = output;

            t.shift_xr<1>(0);

        }

        static uint8_t arm_io(uint8_t ir, uint8_t *data, uint8_t bitcount)
        {
            uint8_t ir_ack = set_ir(ir);
            set_dr(data, bitcount);
            return ir_ack;
        }

    };

}

#endif // TINY_JTAG_H
