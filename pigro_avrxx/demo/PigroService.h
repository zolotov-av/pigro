#ifndef PIGRO_SERVICE_H
#define PIGRO_SERVICE_H

#include <avrxx/spi_master.h>

#include "PigroProto.h"
#include "stm32.h"

avr::SPI_Master spi {};

class PigroService: public PigroProto
{
public:

    static constexpr uint8_t PROTO_VERSION = 0;
    static constexpr uint8_t SERVICE_VERSION = 1;

    /**
     * Обработка команды seta
     */
    static void cmd_seta()
    {
        //if ( pkt.len == 1 ) PORTA = pkt.data[0];
        if ( pkt.len == 2 )
        {
            pkt.data[0] = PROTO_VERSION;
            pkt.data[1] = SERVICE_VERSION;
            send_packet();
        }
    }

    /**
     * Обработка команды isp_reset
     */
    static void cmd_isp_reset()
    {
        if ( pkt.len == 1 )
        {
            avr::pin(PORTB, PB1).set(pkt.data[0]);
        }
    }

    /**
     * Обработка команды isp_io
     */
    static void cmd_isp_io()
    {
        if ( pkt.len == 4 )
        {
            spi.ioctl(pkt.data, 4);
            send_packet();
        }
    }

    static void cmd_jtag_test()
    {
        if ( pkt.len == 1 )
        {
            switch (pkt.data[0])
            {
            case 0:
                STM32::init_jtag();
                return;
            case 1:
                STM32::reset_target(1);
                return;
            case 2:
                STM32::reset_target(0);
                return;
            }
        }
    }

    static void send_error(STM32::error_t error)
    {
        pkt.len = 1;
        pkt.data[0] = error;
        send_packet();
    }

    static void cmd_jtag_raw_ir()
    {
        if ( pkt.len < 2 || (pkt.len-1) * 8 < pkt.data[0] ) return;
        STM32::raw_ir(&pkt.data[1], pkt.data[0]);
        send_packet();
    }

    static void cmd_jtag_raw_dr()
    {
        if ( pkt.len < 2 || (pkt.len-1) * 8 < pkt.data[0] ) return;
        STM32::raw_dr(&pkt.data[1], pkt.data[0]);
        send_packet();
    }

    static void cmd_arm_raw_io()
    {
        if ( pkt.len < 3 || (pkt.len-2) * 8 < pkt.data[1] ) return;
        pkt.data[0] = STM32::raw_io(pkt.data[0], &pkt.data[2], pkt.data[1]);
        send_packet();
    }

    static void cmd_arm_xpacc()
    {
        if ( pkt.len != 6 ) return;

        pkt.data[1] = STM32::xpacc(pkt.data[0], pkt.data[1], reinterpret_cast<uint32_t*>(&pkt.data[2]));
        send_packet();
    }

    static void cmd_arm_apacc()
    {
        if ( pkt.len != 6 ) return;

        pkt.data[1] = STM32::apacc(pkt.data[0], pkt.data[1], reinterpret_cast<uint32_t*>(&pkt.data[2]));
        send_packet();
    }

    static void cmd_arm_config()
    {
        if ( pkt.len == 2 && pkt.data[0] == 1 )
        {
            STM32::memap = pkt.data[1];
            return send_packet();
        }

        if ( pkt.len == 5 && pkt.data[0] == 2 )
        {
            STM32::mem_addr = *reinterpret_cast<uint32_t*>(&pkt.data[1]);
            return send_packet();
        }
    }

    static void cmd_arm_read_next()
    {
        if ( pkt.len == 4 )
        {
            uint32_t value;
            auto error = STM32::read_mem32(STM32::mem_addr, value);
            STM32::mem_addr += 4;
            if ( error ) return send_error(error);
            *reinterpret_cast<uint32_t*>(pkt.data) = value;
            return send_packet();
        }

        if ( pkt.len == 2 )
        {
            uint16_t value;
            auto error = STM32::read_mem16(STM32::mem_addr, value);
            STM32::mem_addr += 2;
            if ( error ) return send_error(error);
            *reinterpret_cast<uint16_t*>(pkt.data) = value;
            return send_packet();
        }
    }

    static void cmd_arm_write_next()
    {
        if ( pkt.len == 4 )
        {
            auto error = STM32::write_mem32(STM32::mem_addr, *reinterpret_cast<uint32_t*>(pkt.data));
            STM32::mem_addr += 4;
            if ( error ) return send_error(error);
            return send_packet();
        }

        if ( pkt.len == 2 )
        {
            auto error = STM32::write_mem16(STM32::mem_addr, *reinterpret_cast<uint16_t*>(pkt.data));
            STM32::mem_addr += 2;
            if ( error ) return send_error(error);
            return send_packet();
        }
    }

    static void cmd_arm_program_next()
    {
        if ( pkt.len == 4 )
        {
            const uint16_t word0 = *reinterpret_cast<uint16_t*>(&pkt.data[0]);
            const uint16_t word1 = *reinterpret_cast<uint16_t*>(&pkt.data[2]);

            uint8_t status0 = STM32::program_flash(STM32::mem_addr+0, word0);
            uint8_t status1 = STM32::program_flash(STM32::mem_addr+2, word1);
            STM32::mem_addr += 4;

            if ( status0 || status1 )
            {
                pkt.len = 2;
                pkt.data[0] = status0;
                pkt.data[1] = status1;
            }

            return send_packet();
        }
    }

    static void cmd_arm_read()
    {
        if ( pkt.len == 8 )
        {
            const uint32_t addr = *reinterpret_cast<uint32_t*>(&pkt.data[0]);
            uint32_t value;
            auto error = STM32::read_mem32(addr, value);
            if ( error ) return send_error(error);

            *reinterpret_cast<uint32_t*>(&pkt.data[4]) = value;
            return send_packet();
        }

        if ( pkt.len == 6 )
        {
            const uint32_t addr = *reinterpret_cast<uint32_t*>(&pkt.data[0]);
            uint16_t value;
            auto error = STM32::read_mem16(addr, value);
            if ( error ) return send_error(error);

            *reinterpret_cast<uint16_t*>(&pkt.data[4]) = value;
            return send_packet();
        }
    }

    static void cmd_arm_write()
    {
        if ( pkt.len == 8 )
        {
            const uint32_t addr = *reinterpret_cast<uint32_t*>(&pkt.data[0]);
            auto error = STM32::write_mem32(addr, *reinterpret_cast<uint32_t*>(&pkt.data[4]));
            if ( error ) return send_error(error);
            return send_packet();
        }

        if ( pkt.len == 6 )
        {
            const uint32_t addr = *reinterpret_cast<uint32_t*>(&pkt.data[0]);
            auto error = STM32::write_mem16(addr, *reinterpret_cast<uint16_t*>(&pkt.data[4]));
            if ( error ) return send_error(error);
            return send_packet();
        }
    }

    /**
     * Обработка команд
     */
    static void handle_packet()
    {
        switch( pkt.cmd )
        {
        case 1:
            cmd_seta();
            return;
        case 2:
            cmd_isp_reset();
            return;
        case 3:
            cmd_isp_io();
            return;
        case 4:
            //cmd_adc();
            return;
        case 5:
            cmd_jtag_test();
            return;
        case 6:
            cmd_jtag_raw_ir();
            return;
        case 7:
            cmd_jtag_raw_dr();
            return;
        case 8:
            cmd_arm_raw_io();
            return;
        case 9:
            cmd_arm_xpacc();
            return;
        case 10:
            cmd_arm_apacc();
            return;
        case 11:
            cmd_arm_config();
            return ;
        case 12:
            cmd_arm_read_next();
            return;
        case 13:
            cmd_arm_write_next();
            return;
        case 14:
            cmd_arm_program_next();
            return;
        case 15:
            cmd_arm_read();
            return;
        case 16:
            cmd_arm_write();
            return;
        }
    }

    static void run()
    {
        while ( true )
        {
            if ( read_packet() )
            {
                send_ack();
                handle_packet();
            }
            else
            {
                send_nack();
                skip_trash();
            }
        }
    }

};

#endif // PIGRO_SERVICE_H
