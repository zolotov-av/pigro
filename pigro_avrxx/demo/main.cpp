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

#define JTCK avr::pin(PORTA, PA2)
#define JTDI avr::pin(PORTA, PA1)
#define JTDO avr::pin(PORTA, PINA, PA5)
#define JTMS avr::pin(PORTA, PA3)
#define JRST avr::pin(PORTA, PA4)

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

static uint8_t jtag_shift_ir(uint8_t ir)
{
    uint8_t output = 0;
    for(uint8_t i = 0; i < 4; i++)
    {
        JTDI.set(ir & 1);
        ir = ir >> 1;
        jtag_tms(0);
        output = (output >> 1) | (JTDO.value() ? 0x8 : 0);
    }
    return output;
}

static uint8_t jtag_shift_dr(uint8_t dr)
{
    uint8_t output = 0;
    for(uint8_t i = 0; i < 8; i++)
    {
        JTDI.set(dr & 1);
        dr = dr >> 1;
        jtag_tms(0);
        output = (output >> 1) | (JTDO.value() ? 0x80 : 0);
    }
    return output;
}

static uint8_t jtag_set_ir(uint8_t ir)
{
    jtag_tms(0);
    jtag_tms(1);
    jtag_tms(1);
    jtag_tms(0);
    jtag_tms(0);

    uint8_t output = jtag_shift_ir(ir);
    jtag_tms(1); // exit1-ir
    jtag_tms(1); // update-ir
    jtag_tms(0); // idle

    return output;
}

static void jtag_set_dr(uint8_t *data, uint8_t bitcount)
{
    jtag_tms(0); // idle
    jtag_tms(1); // select dr-scan
    jtag_tms(0); // capture dr

    uint8_t dr = data[0];
    uint8_t bit = 0;
    uint8_t output = 0;
    for(uint8_t i = 0; i < bitcount; i++)
    {
        JTDI.set(dr & 1);
        dr = dr >> 1;
        jtag_tms(0);
        output = (output >> 1) | (JTDO.value() ? 0x80 : 0);
        bit ++;
        if ( bit == 8 )
        {
            data[0] = output;
            data++;
            dr = data[0];
            bit = 0;
        }
    }
    if ( bit != 0 )
    {
        data[0] = output;
    }

    jtag_tms(1); // exit1-dr
    jtag_tms(1); // update-dr
    jtag_tms(0); // idle
}

static void cmd_jtag_test()
{
    avr::pin(PORTA, 7).set(1);
    pkt.cmd = 5;
    pkt.len = 5;
    JRST.set(0);
    JRST.set(1);
    pkt.data[0] = jtag_set_ir( 0b1110 );
    pkt.data[1] = 0;
    pkt.data[2] = 0;
    pkt.data[3] = 0;
    pkt.data[4] = 0;
    jtag_set_dr(&pkt.data[1], 32);
    send_packet();
}

static void cmd_jtag_ir()
{
    if ( pkt.len != 1 || pkt.data[0] > 0x0F ) return;

    pkt.data[0] = jtag_set_ir( pkt.data[0] );
    send_packet();
}

static void cmd_jtag_dr()
{
    jtag_set_dr(&pkt.data[1], pkt.data[0]);
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

    DDRA = makebits(PA0, PA1, PA2, PA3, PA4, /*PA5,*/ PA6, PA7); //0xFF;
    PORTA = 1;

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
