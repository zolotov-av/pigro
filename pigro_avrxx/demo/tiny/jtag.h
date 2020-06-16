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

        static constexpr uint8_t IR_BYPASS = 0b1111;
        static constexpr uint8_t IR_IDCODE = 0b1110;
        static constexpr uint8_t IR_DPACC = 0b1010;
        static constexpr uint8_t IR_APACC = 0b1011;
        static constexpr uint8_t IR_ABORT = 0b1000;

        static constexpr uint8_t ACK_OKFAULT = 0b010;
        static constexpr uint8_t ACK_WAIT = 0b001;

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

            return output >> 4;
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

        static void set_dr_xpacc(uint8_t *data)
        {
            JTMS.set(1);
            clk(); // ->select-dr

            transaction t;

            data[0] = t.shift_xr<3>(data[0]) >> 5;
            data[1] = t.shift_xr<8>(data[1]);
            data[2] = t.shift_xr<8>(data[2]);
            data[3] = t.shift_xr<8>(data[3]);
            data[4] = t.shift_xr<8>(data[4]);
            t.shift_xr<1>(0);
        }

        template <uint8_t count>
        static void memcpy(uint8_t *dest, uint8_t *src)
        {
            for(uint8_t i = 0; i < count; i++)
                dest[i] = src[i];
        }

        static uint8_t xpacc_io(uint8_t ir, uint8_t *data)
        {
            const uint8_t ir_ack = set_ir(ir);
            set_dr_xpacc(data);
            return ir_ack;
        }

        static bool xpacc_is_error(const uint8_t *data)
        {
            constexpr uint8_t mask = (1 << 5) | (1 << 4) | (1 << 1);
            return (data[0] != ACK_OKFAULT) || (data[1] & mask);
        }

        static constexpr uint8_t read_reg(uint8_t reg)
        {
            return ((reg / 4) << 1) | 1;
        }

        static constexpr uint8_t write_reg(uint8_t reg)
        {
            return ((reg / 4) << 1) | 0;
        }

        static uint8_t arm_xpacc(uint8_t ir, uint8_t *data)
        {
            // send command
            if ( uint8_t ir_ack = xpacc_io(ir, data); ir_ack != 1 ) return ir_ack;

            // send read status, recv command reply
            data[0] = read_reg(0x4);
            if ( uint8_t ir_ack = xpacc_io(IR_DPACC, data); ir_ack != 1 ) return ir_ack;

            // send read rdbuff, recv status reply
            uint8_t status[5] = {read_reg(0xC)};
            if ( uint8_t ir_ack = xpacc_io(IR_DPACC, status); ir_ack != 1 ) return ir_ack;

            const uint8_t ack = data[0];

            if ( ack == ACK_OKFAULT && !xpacc_is_error(status) )
            {
                return 1;
            }

            memcpy<5>(data, status);

            data[0] |= (1 << 5);

            if ( ack == ACK_WAIT )
            {
                data[0] |= (1 << 3);
                return 1;
            }

            data[0] |= (1 << 4);
            return 1;
        }

        static uint8_t arm_apacc(uint8_t ap, uint8_t *data)
        {
            // SELECT AP
            const uint8_t reg = data[0];
            uint8_t temp[5] = {write_reg(0x8), uint8_t(reg & 0xF0), 0, 0, ap};
            if ( uint8_t ir_ack = arm_xpacc(IR_DPACC, temp); ir_ack != 1 ) return ir_ack;
            if ( temp[0] & (1 << 5) )
            {
                memcpy<5>(data, temp);
                data[0] |= (1 << 6);
                return 1;
            }

            // EXEC AP
            data[0] = reg >> 1;
            if ( uint8_t ir_ack = arm_xpacc(IR_APACC, data); ir_ack != 1 ) return ir_ack;
            return 1;
        }

    };

}

#endif // TINY_JTAG_H
