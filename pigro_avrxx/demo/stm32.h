#ifndef PIGRO_STM32_H
#define PIGRO_STM32_H

#include "jtag.h"

class STM32: public JTAG
{
public:

    ///////////////////////////////////////////////////////////////////
    /// STM32 Debug Port

    static constexpr uint8_t IR_BYPASS = 0b1111;
    static constexpr uint8_t IR_IDCODE = 0b1110;
    static constexpr uint8_t IR_DPACC = 0b1010;
    static constexpr uint8_t IR_APACC = 0b1011;
    static constexpr uint8_t IR_ABORT = 0b1000;

    static constexpr uint8_t ACK_OKFAULT = 0b010;
    static constexpr uint8_t ACK_WAIT = 0b001;

    static inline uint8_t memap;
    static inline uint32_t mem_addr;

    static constexpr uint32_t default_csw = 0x22000000;

    /**
     * Error code
     *
     * First hex-digit is error code itself.
     * Second hex-digit is ACK-code
     *
     * ACK-codes (see ARM Debug Interface Architecture Specification):
     *   0x01 (ACK_WAIT)
     *   0x02 (ACK_OKFAULT)
     *   ... other is a fault (reserved codes)
     *
     * 0x00 - success
     * 0x1X - I/O failure (unexpected reply from shift-ir)
     * 0x2X - command failure (CTRL/STAT reports some sticky flags)
     * 0x4X - select failure (write to SELECT register)
     */
    using error_t = uint8_t;

    /**
     * Control RESET line of target device
     */
    static inline void reset_target(bool value)
    {
        JD_RESET.set(value);
    }

    static inline void init_jtag()
    {
        JTAG::init();
    }

    /**
     * Reset JTAG-SM state
     */
    static inline void reset_jtag_state()
    {
        JTAG::reset();
    }

    /**
     * Transmit instruction register (as-is)
     *
     * Execute sequence: select-dr, select-ir, capture-ir, shift-ir, exit1-ir, update-ir
     */
    static void raw_ir(uint8_t *data, uint8_t bitcount)
    {
        JTMS.set(1);
        clk(); // ->select-dr
        clk(); // ->select-ir
        transaction().shift_ex(data, bitcount);
    }

    /**
     * Transmit data register (as-is)
     *
     * Execute sequence: select-dr, capture-dr, shift-dr, exit1-dr, update-dr
     */
    static void raw_dr(uint8_t *data, uint8_t bitcount)
    {
        JTMS.set(1);
        clk(); // ->select-dr
        transaction().shift_ex(data, bitcount);
    }

    /**
     * Transmit instruction register (stm32)
     *
     * Execute sequence: select-dr, select-ir, capture-ir, shift-ir, exit1-ir, update-ir
     *
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
     * Transmit data register (stm32)
     *
     * на входе TMS=0 (idle) или TMS=1, 2-clk
     * на выходе TMS=1, 2-clk
     */
    static void set_dr(uint8_t *data, uint8_t bitcount)
    {
        JTMS.set(1);
        clk(); // ->select-dr

        transaction t;
        t.shift_ex(data, bitcount);
        t.shift_xr<1>(0);
    }

    /**
     * Execute raw I/O
     */
    static uint8_t raw_io(uint8_t ir, uint8_t *data, uint8_t bitcount)
    {
        uint8_t ir_ack = set_ir(ir);
        set_dr(data, bitcount);
        return ir_ack;
    }

    /**
     * Transmit data register
     *
     * Execute sequence: select-dr, capture-dr, shift-dr, exit1-dr, update-dr
     *
     * @param register (bits[3:2] = reg[3:2]) and command (bits[1])
     * @param buffer (in <-> out)
     *
     * @return ACK-code
     */
    static error_t set_dr_xpacc(uint8_t reg_cmd, uint32_t *value)
    {
        JTMS.set(1);
        clk(); // ->select-dr

        transaction t;

        uint8_t *data = reinterpret_cast<uint8_t*>(value);
        const uint8_t ack = t.shift_xr<3>( reg_cmd >> 1 ) >> 5;
        data[0] = t.shift_xr<8>(data[0]);
        data[1] = t.shift_xr<8>(data[1]);
        data[2] = t.shift_xr<8>(data[2]);
        data[3] = t.shift_xr<8>(data[3]);
        t.shift_xr<1>(0);

        return ack;
    }

    static constexpr uint8_t request_read(uint8_t reg)
    {
        return (reg & 0xFC) | 0b10;
    }

    static constexpr uint8_t request_write(uint8_t reg)
    {
        return (reg & 0xFC) | 0b00;
    }

    static constexpr bool is_error_status(uint32_t status)
    {
        constexpr uint8_t error_mask = (1 << 5) | (1 << 4) | (1 << 1);
        return status & error_mask;
    }

    /**
     * Execute i/o instruction (DPACC or APACC)
     *
     * @param ir-code
     * @param register (bits[3:2] = reg[3:2]) and command (bits[1])
     * @param buffer (in <-> out)
     *
     * @return 0x10 on I/O failure, on success return ACK-code
     */
    static error_t xpacc_io(uint8_t ir, uint8_t reg_cmd, uint32_t *data)
    {
        if ( set_ir(ir) != 1 ) return 0x10;
        return set_dr_xpacc(reg_cmd, data);
    }

    /**
     * Execute read from register (DPACC or APACC)
     *
     * @return 0x10 on I/O failure, on success return ACK-code
     */
    static error_t xpacc_io_read(uint8_t ir, uint8_t reg, uint32_t *data)
    {
        return xpacc_io(ir, request_read(reg), data);
    }

    /**
     * Execute write to register (DPACC or APACC)
     *
     * @return 0x10 on I/O failure, on success return ACK-code
     */
    static error_t xpacc_io_write(uint8_t ir, uint8_t reg, uint32_t *data)
    {
        return xpacc_io(ir, request_write(reg), data);
    }

    /**
     * Execute read/write transaction (DPACC or APACC)
     *
     * @param ir-code
     * @param register (bits[3:2] = reg[3:2]) and command (bits[1]: 1 - read, 0 - write)
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_xpacc(uint8_t ir, uint8_t reg_cmd, uint32_t *data)
    {
        // send command
        if ( xpacc_io(ir, reg_cmd, data) == 0x10 ) return 0x10;

        // send read status, recv command reply
        const uint8_t ack = xpacc_io_read(IR_DPACC, 0x4, data);
        if ( ack == 0x10 ) return 0x10;

        // send read rdbuff, recv status reply
        uint32_t status;
        const uint8_t status_ack = xpacc_io_read(IR_DPACC, 0xC, &status);
        if ( status_ack == 0x10 ) return 0x10;

        if ( ack == ACK_OKFAULT && status_ack == ACK_OKFAULT && !is_error_status(status) )
        {
            return 0;
        }

        *data = status;

        return ack | 0x20;
    }

    /**
     * Read from register (DPACC or APACC)
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_xpacc_read(uint8_t ir, uint8_t reg, uint32_t *data)
    {
        return arm_xpacc(ir, request_read(reg), data);
    }

    /**
     * Write to register (DPACC or APACC)
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_xpacc_write(uint8_t ir, uint8_t reg, uint32_t *data)
    {
        return arm_xpacc(ir, request_write(reg), data);
    }

    /**
     * Read from DPACC register
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_dp_read(uint8_t reg, uint32_t *data)
    {
        return arm_xpacc_read(IR_DPACC, reg, data);
    }

    /**
     * Write to DPACC register
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_dp_write(uint8_t reg, uint32_t *data)
    {
        return arm_xpacc_write(IR_DPACC, reg, data);
    }

    /**
     * Execute read/write transaction (APACC)
     *
     * @param AP number
     * @param register (bits[3:2] = reg[3:2]) and command (bits[1]: 1 - read, 0 - write)
     * @param data to send/recv
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_apacc(uint8_t ap, uint8_t reg_cmd, uint32_t *data)
    {
        // SELECT AP
        uint32_t temp = (uint32_t(ap) << 24) | (reg_cmd & 0xF0);
        if ( auto error = arm_dp_write(0x8, &temp) )
        {
            *data = temp;
            return error | 0x40;
        }

        // EXEC AP
        return arm_xpacc(IR_APACC, reg_cmd, data);
    }

    /**
     * Read from APACC register
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_apacc_read(uint8_t ap, uint8_t reg, uint32_t *data)
    {
        return arm_apacc(ap, reg | 0b10, data);
    }

    /**
     * Write to APACC register
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_apacc_write(uint8_t ap, uint8_t reg, uint32_t *data)
    {
        return arm_apacc(ap, reg | 0b00, data);
    }

    /**
     * Read from MEM-AP register
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_read_memap_reg(uint8_t reg, uint32_t &value)
    {
        return arm_apacc_read(memap, reg, &value);
    }

    /**
     * Write to MEM-AP register
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_write_memap_reg(uint8_t reg, uint32_t value)
    {
        return arm_apacc_write(memap, reg, &value);
    }

    /**
     * Read 32-bit value from memory address
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_mem_read32(uint32_t addr, uint32_t &value)
    {
        if ( auto err = arm_write_memap_reg(0x00, default_csw | 2) ) return err;
        if ( auto err = arm_write_memap_reg(0x04, addr) ) return err;
        if ( auto err = arm_read_memap_reg(0x0C, value) ) return err;
        return 0;
    }

    /**
     * Read 16-bit value from memory address
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_mem_read16(uint32_t addr, uint16_t &value)
    {
        if ( auto err = arm_write_memap_reg(0x00, default_csw | 1) ) return err;
        if ( auto err = arm_write_memap_reg(0x04, addr) ) return err;
        uint32_t output;
        if ( auto err = arm_read_memap_reg(0x0C, output) ) return err;
        value = (addr & 2) ? (output >> 16) : output;
        return 0;
    }

    /**
     * Write 32-bit value to memory address
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_mem_write32(uint32_t addr, uint32_t value)
    {
        if ( auto err = arm_write_memap_reg(0x00, default_csw | 2) ) return err;
        if ( auto err = arm_write_memap_reg(0x04, addr) ) return err;
        if ( auto err = arm_write_memap_reg(0x0C, value) ) return err;
        return 0;
    }

    /**
     * Write 16-bit value to memory address
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_mem_write16(uint32_t addr, uint16_t value)
    {
        if ( auto err = arm_write_memap_reg(0x00, default_csw | 1) ) return err;
        if ( auto err = arm_write_memap_reg(0x04, addr) ) return err;
        const uint32_t data = (addr & 2) ? ((uint32_t(value) << 16)) : (value);
        if ( auto err = arm_write_memap_reg(0x0C, data) ) return err;
        return 0;
    }

    /**
     * Read from FPEC register
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t get_fpec_reg(uint8_t reg, uint32_t &value)
    {
        return arm_mem_read32(0x40022000 + reg, value);
    }

    /**
     * Write to FPEC register
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t set_fpec_reg(uint8_t reg, uint32_t value)
    {
        return arm_mem_write32(0x40022000 + reg, value);
    }

    /**
     * Reset errors in FLASH_SR register
     *
     * @return 0 on success, 0x2X on command failure where X is ACK-code
     */
    static error_t arm_fpec_reset_sr()
    {
        return set_fpec_reg(0x0C, (1 << 2) | (1 << 4) | (1 << 5));
    }

    /**
     * Check errors in FLASH_SR register
     *
     * @return FLASH_SR with additional flags
     *     bit[6] - fail to read FLASH_SR register (all other bits invalid)
     *     bit[7] - unknown error
     *     return 0 on success
     */
    static uint8_t arm_fpec_check_sr()
    {
        uint32_t flash_sr;
        if ( get_fpec_reg(0x0C, flash_sr) ) return (1 << 6);

        constexpr uint8_t error_mask = 1 | (1 << 2) | (1 << 4);
        if ( flash_sr & error_mask ) return flash_sr;
        if ( flash_sr & (1 << 5) ) return 0;
        return flash_sr | (1 << 7);
    }

    /**
     * Write to flash memory
     *
     * @return 0 on success or FLASH_SR with additional flags:
     *     bit[6] - fail to read FLASH_SR register (all other bits invalid)
     *     bit[7] - unknown error
     *     return 0 on success
     */
    static uint8_t arm_fpec_program(uint32_t addr, uint16_t value)
    {
        if ( arm_fpec_reset_sr() ) return (1 << 7);
        if ( arm_mem_write16(addr, value) ) return (1 << 7);
        return arm_fpec_check_sr();
    }

};

#endif // PIGRO_STM32_H
