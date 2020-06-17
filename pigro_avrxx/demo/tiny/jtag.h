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

        static uint8_t set_dr_xpacc(uint8_t cmd, uint32_t *value)
        {
            JTMS.set(1);
            clk(); // ->select-dr

            transaction t;

            uint8_t *data = reinterpret_cast<uint8_t*>(value);
            const uint8_t ack = t.shift_xr<3>(cmd) >> 5;
            data[0] = t.shift_xr<8>(data[0]);
            data[1] = t.shift_xr<8>(data[1]);
            data[2] = t.shift_xr<8>(data[2]);
            data[3] = t.shift_xr<8>(data[3]);
            t.shift_xr<1>(0);

            return ack;
        }

        static constexpr uint8_t request_read(uint8_t reg)
        {
            return ((reg >> 2) << 1) | 1;
        }

        static constexpr uint8_t request_write(uint8_t reg)
        {
            return ((reg >> 2) << 1) | 0;
        }

        static constexpr bool is_error_status(uint32_t status)
        {
            constexpr uint8_t error_mask = (1 << 5) | (1 << 4) | (1 << 1);
            return status & error_mask;
        }

        /**
         * @return 0x10 on I/O failure, on success return ACK
         */
        static uint8_t xpacc_io(uint8_t ir, uint8_t cmd, uint32_t *data)
        {
            if ( set_ir(ir) != 1 ) return 0x10;
            return set_dr_xpacc(cmd, data);
        }

        /**
         * @return 0x10 on I/O failure, on success return ACK
         */
        static uint8_t xpacc_read(uint8_t ir, uint8_t reg, uint32_t *data)
        {
            return xpacc_io(ir, request_read(reg), data);
        }

        /**
         * @return 0x10 on I/O failure, on success return ACK
         */
        static uint8_t xpacc_write(uint8_t ir, uint8_t reg, uint32_t *data)
        {
            return xpacc_io(ir, request_write(reg), data);
        }

        /**
         * @return 0x2X on command failure, on success return ACK_OKFAULT
         */
        static uint8_t arm_xpacc(uint8_t ir, uint8_t cmd, uint32_t *data)
        {
            // send command
            if ( xpacc_io(ir, cmd, data) == 0x10 ) return 0x10;

            // send read status, recv command reply
            const uint8_t ack = xpacc_read(IR_DPACC, 0x4, data);
            if ( ack == 0x10 ) return 0x10;

            // send read rdbuff, recv status reply
            uint32_t status;
            const uint8_t status_ack = xpacc_read(IR_DPACC, 0xC, &status);
            if ( status_ack == 0x10 ) return 0x10;

            if ( ack == ACK_OKFAULT && status_ack == ACK_OKFAULT && !is_error_status(status) )
            {
                return ack;
            }

            *data = status;

            return ack | 0x20;
        }

        /**
         * @return 0x2X on command failure, on success return ACK_OKFAULT
         */
        static uint8_t arm_xpacc_read(uint8_t ir, uint8_t reg, uint32_t *data)
        {
            return arm_xpacc(ir, request_read(reg), data);
        }

        /**
         * @return 0x2X on command failure, on success return ACK_OKFAULT
         */
        static uint8_t arm_xpacc_write(uint8_t ir, uint8_t reg, uint32_t *data)
        {
            return arm_xpacc(ir, request_write(reg), data);
        }

        /**
         * @return 0x2X on command failure, on success return ACK_OKFAULT
         */
        static uint8_t arm_dp_read(uint8_t reg, uint32_t *data)
        {
            return arm_xpacc_read(IR_DPACC, reg, data);
        }

        /**
         * @return 0x2X on command failure, on success return ACK_OKFAULT
         */
        static uint8_t arm_dp_write(uint8_t reg, uint32_t *data)
        {
            return arm_xpacc_write(IR_DPACC, reg, data);
        }

        /**
         * @return 0x2X on command failure, on success return ACK_OKFAULT
         */
        static uint8_t arm_apacc(uint8_t ap, uint8_t reg_cmd, uint32_t *data)
        {
            // SELECT AP
            uint32_t temp = (uint32_t(ap) << 24) | (reg_cmd & 0xF0);
            uint8_t status = arm_dp_write(0x8, &temp);
            if ( status != ACK_OKFAULT )
            {
                *data = temp;
                return status | 0x40;
            }

            // EXEC AP
            return arm_xpacc(IR_APACC, reg_cmd >> 1, data);
        }

        /**
         * @return 0x2X on command failure, on success return ACK_OKFAULT
         */
        static uint8_t arm_apacc_read(uint8_t ap, uint8_t reg, uint32_t *data)
        {
            return arm_apacc(ap, reg | 0b10, data);
        }

        /**
         * @return 0x2X on command failure, on success return ACK_OKFAULT
         */
        static uint8_t arm_apacc_write(uint8_t ap, uint8_t reg, uint32_t *data)
        {
            return arm_apacc(ap, reg | 0b00, data);
        }

        ///////////////////////////////////////////////////////////////////
        /// STM32 MEM-AP

        static inline uint8_t memap;
        static inline uint32_t mem_addr;

        static constexpr uint32_t default_csw = 0x22000000;

        static bool arm_read_memap_reg(uint8_t reg, uint32_t &value)
        {
            return arm_apacc_read(memap, reg, &value);
        }

        static bool arm_write_memap_reg(uint8_t reg, uint32_t value)
        {
            return arm_apacc_write(memap, reg, &value);
        }

        static bool arm_mem_read32(uint32_t addr, uint32_t &value)
        {
            if ( ! arm_write_memap_reg(0x00, default_csw | 2) ) return false;
            if ( ! arm_write_memap_reg(0x04, addr) ) return false;
            if ( ! arm_read_memap_reg(0x0C, value) ) return false;
            return true;
        }

        static bool arm_mem_read16(uint32_t addr, uint16_t &value)
        {
            if ( ! arm_write_memap_reg(0x00, default_csw | 1) ) return false;
            if ( ! arm_write_memap_reg(0x04, addr) ) return false;
            uint32_t output;
            if ( ! arm_read_memap_reg(0x0C, output) ) return false;
            value = (addr & 2) ? (output >> 16) : output;
            return true;
        }

        static bool arm_mem_write32(uint32_t addr, uint32_t value)
        {
            if ( ! arm_write_memap_reg(0x00, default_csw | 2) ) return false;
            if ( ! arm_write_memap_reg(0x04, addr) ) return false;
            if ( ! arm_write_memap_reg(0x0C, value) ) return false;
            return true;
        }

        static bool arm_mem_write16(uint32_t addr, uint16_t value)
        {
            if ( ! arm_write_memap_reg(0x00, default_csw | 1) ) return false;
            if ( ! arm_write_memap_reg(0x04, addr) ) return false;
            const uint32_t data = (addr & 2) ? ((uint32_t(value) << 16)) : (value);
            if ( ! arm_write_memap_reg(0x0C, data) ) return false;
            return true;
        }

        static bool get_fpec_reg(uint8_t reg, uint32_t &value)
        {
            return arm_mem_read32(0x40022000 + reg, value);
        }

        static bool set_fpec_reg(uint8_t reg, uint32_t value)
        {
            return arm_mem_write32(0x40022000 + reg, value);
        }

        static bool arm_fpec_reset_sr()
        {
            return set_fpec_reg(0x0C, (1 << 2) | (1 << 4) | (1 << 5));
        }

        static uint8_t arm_fpec_check_sr()
        {
            uint32_t flash_sr;
            if ( !get_fpec_reg(0x0C, flash_sr) ) return (1 << 6);

            constexpr uint8_t error_mask = 1 | (1 << 2) | (1 << 4);
            if ( flash_sr & error_mask ) return flash_sr;
            if ( flash_sr & (1 << 5) ) return 0;
            return flash_sr;
        }

        static uint8_t arm_fpec_program(uint32_t addr, uint16_t value)
        {
            if ( !arm_fpec_reset_sr() ) return (1 << 6);
            if ( !arm_mem_write16(addr, value) ) return (1 << 6);
            return arm_fpec_check_sr();
        }

    };

}

#endif // TINY_JTAG_H
