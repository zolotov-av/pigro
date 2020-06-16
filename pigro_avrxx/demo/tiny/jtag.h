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

        ///////////////////////////////////////////////////////////////////
        /// STM32 MEM-AP

        static inline uint8_t memap;
        static inline uint32_t mem_addr;

        static constexpr uint32_t default_csw = 0x22000000;

        static void set_mem_addr(uint8_t *data)
        {
            mem_addr = data[0] | (uint32_t(data[1]) << 8) | (uint32_t(data[2]) << 16) | (uint32_t(data[3]) << 24);
        }

        static bool arm_read_memap_reg(uint8_t reg, uint32_t &value)
        {
            uint8_t data[5];
            data[0] = (reg & 0xFC) | 0b10;
            data[1] = value & 0xFF;
            data[2] = (value >> 8) & 0xFF;
            data[3] = (value >> 16) & 0xFF;
            data[4] = (value >> 24) & 0xFF;
            if ( arm_apacc(memap, data) != 1 ) return false;
            if ( data[0] & 0xFC ) return false;
            value = data[1] | (uint32_t(data[2]) << 8) | (uint32_t(data[3]) << 16) | (uint32_t(data[4]) << 24);
            return true;
        }

        static bool arm_write_memap_reg(uint8_t reg, const uint32_t &value)
        {
            uint8_t data[5];
            data[0] = (reg & 0xFC) | 0b00;
            data[1] = value & 0xFF;
            data[2] = (value >> 8) & 0xFF;
            data[3] = (value >> 16) & 0xFF;
            data[4] = (value >> 24) & 0xFF;
            if ( arm_apacc(memap, data) != 1 ) return false;
            return (data[0] & 0xFC) == 0;
        }

        static bool arm_mem_read32(const uint32_t &addr, uint32_t &value)
        {
            if ( ! arm_write_memap_reg(0x00, default_csw | 2) ) return false;
            if ( ! arm_write_memap_reg(0x04, addr) ) return false;
            if ( ! arm_read_memap_reg(0x0C, value) ) return false;
            return true;
        }

        static bool arm_mem_read16(const uint32_t &addr, uint16_t &value)
        {
            if ( ! arm_write_memap_reg(0x00, default_csw | 1) ) return false;
            if ( ! arm_write_memap_reg(0x04, addr) ) return false;
            uint32_t output;
            if ( ! arm_read_memap_reg(0x0C, output) ) return false;
            value = (addr & 0x02) ? (output >> 16) : output;
            return true;
        }

        static bool arm_mem_write32(const uint32_t &addr, const uint32_t &value)
        {
            if ( ! arm_write_memap_reg(0x00, default_csw | 2) ) return false;
            if ( ! arm_write_memap_reg(0x04, addr) ) return false;
            if ( ! arm_write_memap_reg(0x0C, value) ) return false;
            return true;
        }

        static bool arm_mem_write16(const uint32_t &addr, const uint16_t &value)
        {
            if ( ! arm_write_memap_reg(0x00, default_csw | 1) ) return false;
            if ( ! arm_write_memap_reg(0x04, addr) ) return false;
            uint32_t data = (addr & 2) ? ((uint32_t(value) << 16)) : (value);
            if ( ! arm_write_memap_reg(0x0C, data) ) return false;
            return true;
        }

        static bool get_fpec_reg(uint8_t reg, uint32_t &value)
        {
            return arm_mem_read32(0x40022000 + reg, value);
        }

        static bool set_fpec_reg(uint8_t reg, const uint32_t &value)
        {
            return arm_mem_write32(0x40022000 + reg, value);
        }

        static bool arm_fpec_reset_sr()
        {
            return set_fpec_reg(0x0C, (1 << 2) | (1 << 4) | (1 << 5));
        }

        static bool arm_fpec_check_sr(uint8_t &status)
        {
            status = 0;
            uint32_t flash_sr;
            if ( !get_fpec_reg(0x0C, flash_sr) ) return false;

            uint8_t mask = 1 | (1 << 2) | (1 << 4);
            if ( flash_sr & mask )
            {
                status = flash_sr;
                return false;
            }

            if ( flash_sr & (1 << 5) )
            {
                return true;
            }

            status = flash_sr;
            return false;
        }

        static bool arm_fpec_program(const uint32_t &addr, const uint16_t &value, uint8_t &status)
        {
            status = 1 << 6;
            if ( !arm_fpec_reset_sr() ) return false;
            if ( !arm_mem_write16(addr, value) ) return false;
            return arm_fpec_check_sr(status);
        }

    };

}

#endif // TINY_JTAG_H
