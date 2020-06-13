#include <avrxx/io.h>
#include <avrxx/spi_master.h>
#include <avrxx/uartbuf.h>
#include <avr/io.h>
#include <avr/interrupt.h>

using namespace avr;

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
uartbuf<8, 8> uart {};

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
    uart.handle_rxc_isr();
}

/**
* USART Transmit Complete
*/
ISR(USART_TXC_vect)
{
    uart.handle_txc_isr();
}

/**
* USART Data Register Empty
*/
ISR(USART_UDRE_vect)
{
    uart.handle_udre_isr();
}

static packet_t pkt;

void send_packet();

static void send_ack()
{
    uart.write_sync(PKT_ACK);
}

static void send_nack()
{
    uart.write_sync(PKT_NACK);
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
 * Отправить пакет сформированный в переменной ptk
 */
void send_packet()
{
    uart.write_sync(pkt.cmd);
    uart.write_sync(pkt.len);
    int i;
    for(i = 0; i < pkt.len; i++)
    {
        uart.write_sync(pkt.data[i]);
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

constexpr uint8_t JTAG_DEFAULT_STATE = makebits(PA0/*RESET*/, PA1/*JDI*/, PA3/*TMS*/);

#define JTCK avr::pin(PORTA, PA2)
#define JTDI avr::pin(PORTA, PA1)
#define JTDO avr::pin(PORTA, PINA, PA5)
#define JTMS avr::pin(PORTA, PA3)
#define JRST avr::pin(PORTA, PA4)
#define JDBG avr::pin(PORTA, PA6)

inline void jtag_clk()
{
    JTCK.set(true);
    JTCK.set(false);
}

inline void jtag_tms(uint8_t tms)
{
    JTMS.set(tms);
    jtag_clk();
}

inline void jtag_reset()
{
    PORTA = JTAG_DEFAULT_STATE;
    JRST.set(1);

}

/**
 * включает JTAG
 * на выходе TMS=1, JRST=1, JDI=1
 */
inline void jtag_begin()
{
    jtag_reset();
}

/**
 * выключает JTAG
 * на выходе TMS=1, JRST=0, JDI=1
 */
inline void jtag_end()
{
    PORTA = JTAG_DEFAULT_STATE;
}

/**
 * на входе TMS=1
 * на выходе TMS=1, 2-clk
 */
static void jtag_shift(uint8_t *data, uint8_t bitcount)
{
    JTMS.set(0);
    jtag_clk(); // capture

    uint8_t dr = *data;
    uint8_t bit = 0;
    uint8_t output = 0;
    for(uint8_t i = 0; i < bitcount; i++)
    {
        jtag_clk();

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
        *data = output;
    }

    JTMS.set(1);
    jtag_clk(); // exit1
    JTDI.set(1); // default state
    jtag_clk(); // update
}

/**
 * на входе TMS=0 (idle)
 * на выходе TMS=1, 2-clk
 */
static uint8_t jtag_set_ir(uint8_t ir)
{
    JTMS.set(1);
    jtag_clk(); // ->select-dr
    jtag_clk(); // ->select-ir
    uint8_t buf[2];
    buf[0] = ir | 0xF0;
    buf[1] = 0xFF;
    jtag_shift(buf, 9); // TMS=1, 2-clk
    return buf[0];
}

/**
 * на входе TMS=0 (idle) или TMS=1, 2-clk
 * на выходе TMS=1, 2-clk
 */
static void jtag_set_dr(uint8_t *data, uint8_t bitcount)
{
    JTMS.set(1);
    jtag_clk(); // ->select-dr
    jtag_shift(data, bitcount+1); // TMS=1, 2-clk
}

static void cmd_jtag_test()
{
    jtag_begin();
}

static void cmd_jtag_ir()
{
    if ( pkt.len != 1 || pkt.data[0] > 0x0F ) return;

    jtag_tms(0); // [Reset/Idle]->idle
    pkt.data[0] = jtag_set_ir(pkt.data[0]);
    jtag_tms(0); // ->idle

    send_packet();
}

static void cmd_jtag_dr()
{
    if ( pkt.len < 2 || pkt.data[0] > 39 ) return;

    jtag_tms(0); // idle
    jtag_set_dr(&pkt.data[1], pkt.data[0]);
    jtag_tms(0); // idle

    send_packet();
}

static void cmd_arm_idcode()
{
    if ( pkt.len != 5 ) return;

    jtag_begin();
    jtag_tms(0); // Reset->idle

    JTMS.set(1);
    jtag_clk(); // ->select-dr
    jtag_clk(); // ->select-ir
    uint8_t buf[2];
    buf[0] = pkt.data[0] | 0xF0;
    buf[1] = 0xFF;
    jtag_shift(buf, 9); // TMS=1, 2-clk
    pkt.data[0] = buf[0];

    jtag_clk(); // select dr-scan
    jtag_shift(&pkt.data[1], 32); // TMS=1, 2-clk

    jtag_end();

    send_packet();
}

static void cmd_arm_xpacc()
{
    if ( pkt.len != 6 ) return;

    jtag_tms(0); // [Reset/Idle]->idle

    const uint8_t ir = pkt.data[0];
    const bool is_read = (pkt.data[1] & 1) == 1;
    pkt.data[0] = jtag_set_ir(ir); // TMS=1, 2-clk

    jtag_set_dr(&pkt.data[1], 35);
    if ( is_read && (pkt.data[1] & 0x7) == 0b010 )
    {
        jtag_set_dr(&pkt.data[1], 35);
    }

    jtag_tms(0); // idle

    send_packet();
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
    }
}

/**
 * Чтение пакета в блокируещем режиме и его обработка
 */
static void read_packet()
{
    int i;
    uart.read_sync(&pkt.cmd);
    uart.read_sync(&pkt.len);

    if ( pkt.len > PACKET_MAXLEN )
    {
        uint8_t dummy;
        send_nack();
        for(i = 0; i < pkt.len; i++) uart.read_sync(&dummy);
        return;
    }

    for(i = 0; i < pkt.len; i++)
    {
        uart.read_sync(&pkt.data[i]);
    }
    send_ack();

    handle_packet();
}

int main()
{

    // Инициализация SPI в режиме мастера
    // PB1 управляет RESET-ом прошиваемого контроллера
    DDRB = makebits(MOSI_BIT, PB7, SS_BIT, PB1);
    SPCR = makebits(SPIE, SPE, MSTR, SPI2X, SPR1, SPR0);

    DDRA = makebits(PA0, PA1, PA2, PA3, PA4, /*PA5,*/ PA6, PA7);
    PORTA = JTAG_DEFAULT_STATE;

    avr::pin(MCUCR, SE).set(true);

    // Настройка UART
    UART::init();
    UART::setBaudRate(UART_BAUD_K);

    avr::interrupt_enable();

    while ( true )
    {
        read_packet();
    }
}
