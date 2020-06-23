#ifndef PIGRO_PROTO_H
#define PIGRO_PROTO_H

#include <stdint.h>
#include <avrxx/uart.h>
#include <tiny/uartbuf.h>

#include "PigroTimer.h"

inline tiny::uartbuf<avr::UART> uart {};

class PigroProto
{
public:

    static constexpr uint8_t PACKET_MAXLEN = 6;

    static constexpr uint8_t PKT_ACK = 1;
    static constexpr uint8_t PKT_NACK = 2;

    struct packet_t
    {
        uint8_t cmd;
        uint8_t len;
        uint8_t data[PACKET_MAXLEN];
    };

    class ReadTimer
    {
    public:
        ReadTimer() { Timer::start(); }
        ~ReadTimer() { Timer::stop(); }
    };

    static inline packet_t pkt;

    static bool usart_read(uint8_t &dest)
    {
        while ( !uart.read(&dest) )
        {
            if ( Timer::expire() ) return false;
            tiny::sleep();
        }
        return true;
    }

    static bool usart_write(uint8_t dest)
    {
        while ( !uart.write(dest) )
        {
            if ( Timer::expire() ) return false;
            tiny::sleep();
        }
        return true;
    }

    static inline bool send_ack()
    {
        return usart_write(PKT_ACK);
    }

    static inline bool send_nack()
    {
        return usart_write(PKT_NACK);
    }

    static bool read_packet()
    {

        // первый пакет читаем без таймаута
        uart.read_sync(&pkt.cmd);

        // для последующих байт задаем таймаут
        ReadTimer tm;

        if ( ! usart_read(pkt.len) ) return false;

        if ( pkt.len > PACKET_MAXLEN ) return false;

        for(uint8_t i = 0; i < pkt.len; i++)
        {
            if ( !usart_read(pkt.data[i]) ) return false;
        }

        return true;
    }

    static void send_packet()
    {
        if ( !usart_write(pkt.cmd) ) return;
        if ( !usart_write(pkt.len) ) return;
        for(uint8_t i = 0; i < pkt.len; i++)
        {
            if ( !usart_write(pkt.data[i]) ) return;
        }
        return;
    }

    /**
     * Прочитать мусор после рассинхрона
     */
    static void skip_trash()
    {
        ReadTimer tm;
        uint8_t dummy;
        while ( !Timer::expire() )
        {
            while ( uart.read(&dummy) ) Timer::reset();
            tiny::sleep();
        }
    }

};

#endif // PIGRO_PROTO_H
