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
            JTAG::reset(pkt.data[0]);
        }
    }

    static void send_error(STM32::error_t error)
    {
        pkt.len = 1;
        pkt.data[0] = error;
        send_packet();
    }

    static void cmd_jtag_ir()
    {
        if ( pkt.len != 1 || pkt.data[0] > 0x0F ) return;

        JTAG::tms(0); // [Reset/Idle]->idle
        pkt.data[0] = STM32::set_ir(pkt.data[0]);
        JTAG::tms(0); // ->idle

        send_packet();
    }

    static void cmd_jtag_dr()
    {
        if ( pkt.len < 2 || pkt.data[0] > 39 ) return;

        JTAG::tms(0); // idle
        STM32::set_dr(&pkt.data[1], pkt.data[0]);
        JTAG::tms(0); // idle

        send_packet();
    }

    static void cmd_arm_idcode()
    {
        if ( pkt.len < 3 || pkt.len * 8 < pkt.data[1] ) return;

        JTAG::tms(0); // Reset->idle
        pkt.data[0] = STM32::arm_io(pkt.data[0], &pkt.data[2], pkt.data[1]);
        JTAG::tms(0); // idle

        send_packet();
    }

    static void cmd_arm_xpacc()
    {
        if ( pkt.len != 6 ) return;

        JTAG::tms(0); // [Reset/Idle]->idle
        pkt.data[1] = STM32::arm_xpacc(pkt.data[0], pkt.data[1], reinterpret_cast<uint32_t*>(&pkt.data[2]));
        JTAG::tms(0); // idle

        send_packet();
    }

    static void cmd_arm_apacc()
    {
        if ( pkt.len != 6 ) return;

        JTAG::tms(0); // [Reset/Idle]->idle
        pkt.data[1] = STM32::arm_apacc(pkt.data[0], pkt.data[1], reinterpret_cast<uint32_t*>(&pkt.data[2]));
        JTAG::tms(0); // idle

        send_packet();
    }

    static void cmd_arm_config()
    {
        if ( pkt.len == 2 && pkt.data[0] == 1 )
        {
            STM32::memap = pkt.data[1];
            send_packet();
            return;
        }

        if ( pkt.len == 5 && pkt.data[0] == 2 )
        {
            STM32::mem_addr = *reinterpret_cast<uint32_t*>(&pkt.data[1]);
            send_packet();
            return;
        }
    }

    static void cmd_arm_read()
    {
        if ( pkt.len == 4 )
        {
            uint32_t value;
            auto error = STM32::arm_mem_read32(STM32::mem_addr, value);
            STM32::mem_addr += 4;
            if ( error ) return send_error(error);

            *reinterpret_cast<uint32_t*>(pkt.data) = value;
            send_packet();
            return;
        }

        if ( pkt.len == 2 )
        {
            uint16_t value;
            auto error = STM32::arm_mem_read16(STM32::mem_addr, value);
            STM32::mem_addr += 2;
            if ( error ) return send_error(error);

            *reinterpret_cast<uint16_t*>(pkt.data) = value;
            send_packet();
            return;

        }
    }

    static void cmd_arm_write()
    {
        if ( pkt.len == 4 )
        {
            auto error = STM32::arm_mem_write32(STM32::mem_addr, *reinterpret_cast<uint32_t*>(pkt.data));
            STM32::mem_addr += 4;
            if ( error ) return send_error(error);

            send_packet();
            return;
        }

        if ( pkt.len == 2 )
        {
            auto error = STM32::arm_mem_write16(STM32::mem_addr, *reinterpret_cast<uint16_t*>(pkt.data));
            STM32::mem_addr += 2;
            if ( error ) return send_error(error);

            send_packet();
            return;
        }
    }

    static void cmd_arm_fpec()
    {
        if ( pkt.len == 4 )
        {
            const uint16_t word0 = *reinterpret_cast<uint16_t*>(&pkt.data[0]);
            const uint16_t word1 = *reinterpret_cast<uint16_t*>(&pkt.data[2]);

            uint8_t status0 = STM32::arm_fpec_program(STM32::mem_addr+0, word0);
            uint8_t status1 = STM32::arm_fpec_program(STM32::mem_addr+2, word1);
            STM32::mem_addr += 4;

            if ( status0 || status1 )
            {
                pkt.len = 2;
                pkt.data[0] = status0;
                pkt.data[1] = status1;
            }

            send_packet();
            return;
        }
    }

    static void cmd_arm_reset()
    {
        if ( pkt.len == 1 )
        {
            avr::pin(PORTB, PB1).set(pkt.data[0]);
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
            cmd_jtag_ir();
            return;
        case 7:
            cmd_jtag_dr();
            return;
        case 8:
            cmd_arm_idcode();
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
            cmd_arm_read();
            return;
        case 13:
            cmd_arm_write();
            return;
        case 14:
            cmd_arm_fpec();
            return;
        case 15:
            cmd_arm_reset();
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
