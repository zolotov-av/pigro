#include <tiny/system.h>
#include <avrxx/io.h>
#include <avrxx/spi_master.h>
#include <tiny/uartbuf.h>
#include <avrxx/uart.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <tiny/jtag.h>
#include "timer.h"

using namespace avr;
using namespace tiny;

// 51 at 8MHz = 9600
//constexpr auto UART_BAUD_K = 51;

// 47 at 7.3728 MHz = 9600
constexpr auto UART_BAUD_K = 47;

// 103 at 16MHz = 9600
//constexpr auto UART_BAUD_K = 103;

constexpr auto PACKET_MAXLEN = 6;

constexpr uint8_t PKT_ACK = 1;
constexpr uint8_t PKT_NACK = 2;

struct packet_t
{
    uint8_t cmd;
    uint8_t len;
    uint8_t data[PACKET_MAXLEN];
};

SPI_Master spi {};
tiny::uartbuf<avr::UART> uart {};

class ReadTimer
{
public:
    ReadTimer() { Timer::start(); }
    ~ReadTimer() { Timer::stop(); }
};

/**
 * Прерываение SPI
 */
ISR(SPI_STC_vect)
{
    spi.handle_isr();
}

/**
 * USART Receive Complete
 */
ISR(USART_RXC_vect)
{
    uart.isr_rx_ready();
}

/**
* USART Data Register Empty
*/
ISR(USART_UDRE_vect)
{
    uart.isr_tx_empty();
}

ISR(TIMER0_COMP_vect )
{
    Timer::isr();
}

static packet_t pkt;

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

static bool send_ack()
{
    return usart_write(PKT_ACK);
}

static bool send_nack()
{
    return usart_write(PKT_NACK);
}

/**
 * Отправить пакет сформированный в переменной ptk
 */
bool send_packet()
{
    if ( !usart_write(pkt.cmd) ) return false;
    if ( !usart_write(pkt.len) ) return false;
    for(uint8_t i = 0; i < pkt.len; i++)
    {
        if ( !usart_write(pkt.data[i]) ) return false;
    }
    return true;
}

/**
 * Обработка команды seta
 */
void cmd_seta()
{
    //if ( pkt.len == 1 ) PORTA = pkt.data[0];
    if ( pkt.len == 2 )
    {
        pkt.data[0] = 0;
        pkt.data[1] = 2;
        send_packet();
    }
}

/**
 * Обработка команды isp_reset
 */
void cmd_isp_reset()
{
    if ( pkt.len == 1 )
    {
        avr::pin(PORTB, PB1).set(pkt.data[0]);
    }
}

/**
 * Обработка команды isp_io
 */
void cmd_isp_io()
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

static void send_error(JTAG::error_t error)
{
    pkt.len = 1;
    pkt.data[0] = error;
    send_packet();
}

static void cmd_jtag_ir()
{
    if ( pkt.len != 1 || pkt.data[0] > 0x0F ) return;

    JTAG::tms(0); // [Reset/Idle]->idle
    pkt.data[0] = JTAG::set_ir(pkt.data[0]);
    JTAG::tms(0); // ->idle

    send_packet();
}

static void cmd_jtag_dr()
{
    if ( pkt.len < 2 || pkt.data[0] > 39 ) return;

    JTAG::tms(0); // idle
    JTAG::set_dr(&pkt.data[1], pkt.data[0]);
    JTAG::tms(0); // idle

    send_packet();
}

static void cmd_arm_idcode()
{
    if ( pkt.len < 3 || pkt.len * 8 < pkt.data[1] ) return;

    JTAG::tms(0); // Reset->idle
    pkt.data[0] = JTAG::arm_io(pkt.data[0], &pkt.data[2], pkt.data[1]);
    JTAG::tms(0); // idle

    send_packet();
}

static void cmd_arm_xpacc()
{
    if ( pkt.len != 6 ) return;

    JTAG::tms(0); // [Reset/Idle]->idle
    pkt.data[1] = JTAG::arm_xpacc(pkt.data[0], pkt.data[1], reinterpret_cast<uint32_t*>(&pkt.data[2]));
    JTAG::tms(0); // idle

    send_packet();
}

static void cmd_arm_apacc()
{
    if ( pkt.len != 6 ) return;

    JTAG::tms(0); // [Reset/Idle]->idle
    pkt.data[1] = JTAG::arm_apacc(pkt.data[0], pkt.data[1], reinterpret_cast<uint32_t*>(&pkt.data[2]));
    JTAG::tms(0); // idle

    send_packet();
}

static void cmd_arm_config()
{
    if ( pkt.len == 2 && pkt.data[0] == 1 )
    {
        JTAG::memap = pkt.data[1];
        send_packet();
        return;
    }

    if ( pkt.len == 5 && pkt.data[0] == 2 )
    {
        JTAG::mem_addr = *reinterpret_cast<uint32_t*>(&pkt.data[1]);
        send_packet();
        return;
    }
}

static void cmd_arm_read()
{
    if ( pkt.len == 4 )
    {
        uint32_t value;
        auto error = JTAG::arm_mem_read32(JTAG::mem_addr, value);
        JTAG::mem_addr += 4;
        if ( error ) return send_error(error);

        *reinterpret_cast<uint32_t*>(pkt.data) = value;
        send_packet();
        return;
    }

    if ( pkt.len == 2 )
    {
        uint16_t value;
        auto error = JTAG::arm_mem_read16(JTAG::mem_addr, value);
        JTAG::mem_addr += 2;
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
        auto error = JTAG::arm_mem_write32(JTAG::mem_addr, *reinterpret_cast<uint32_t*>(pkt.data));
        JTAG::mem_addr += 4;
        if ( error ) return send_error(error);

        send_packet();
        return;
    }

    if ( pkt.len == 2 )
    {
        auto error = JTAG::arm_mem_write16(JTAG::mem_addr, *reinterpret_cast<uint16_t*>(pkt.data));
        JTAG::mem_addr += 2;
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

        uint8_t status0 = JTAG::arm_fpec_program(JTAG::mem_addr+0, word0);
        uint8_t status1 = JTAG::arm_fpec_program(JTAG::mem_addr+2, word1);
        JTAG::mem_addr += 4;

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
void handle_packet()
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

/**
 * Чтение пакета в блокируещем режиме и его обработка
 */
static bool read_packet()
{

    uart.read_sync(&pkt.cmd);

    ReadTimer tm;

    if ( ! usart_read(pkt.len) ) return false;

    if ( pkt.len > PACKET_MAXLEN )
    {
        uint8_t dummy;
        for(uint8_t i = 0; i < pkt.len; i++)
        {
            if ( !usart_read(dummy) ) return false;
        }
        return false;
    }

    for(uint8_t i = 0; i < pkt.len; i++)
    {
        if ( !usart_read(pkt.data[i]) ) return false;
    }
    return true;
}

/**
 * Прочитать "мусор" после рассинхрона
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

int main()
{

    // Инициализация SPI в режиме мастера
    // PB1 управляет RESET-ом прошиваемого контроллера
    DDRB = makebits(MOSI_BIT, PB7, SS_BIT, PB1);
    SPCR = makebits(SPIE, SPE, MSTR, SPI2X, SPR1, SPR0);

    DDRA = makebits(PA0, PA1, PA2, PA3, PA4, /*PA5,*/ PA6, PA7);
    PORTA = JTAG_DEFAULT_STATE;

    //DDRA = 0xFF;
    //PORTA = 0xFF;

    avr::pin(MCUCR, SE).set(true);

    // Настройка UART
    UART::init();
    UART::setBaudRate(UART_BAUD_K);

    tiny::interrupt_enable();

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
