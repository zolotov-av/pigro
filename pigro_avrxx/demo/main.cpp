#include <avrxx/io.h>
#include <avrxx/spi_master.h>
#include <avrxx/uartbuf.h>
#include <avr/io.h>
#include <avr/interrupt.h>

using namespace avr;

// 51 at 8MHz = 9600
constexpr auto UART_BAUD_K = 51;

constexpr auto PACKET_MAXLEN = 6;

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

/**
 * Обработка команды seta
 */
void cmd_seta()
{
    if ( pkt.len == 1 ) PORTA = pkt.data[0];
}

/**
 * Обработка команды isp_reset
 */
void cmd_isp_reset()
{
    PORTA = 4;
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
    PORTA = 6;
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
    PORTA = 5;
    if ( pkt.len == 4 )
    {
        spi.ioctl(pkt.data, 4);
        send_packet();
    }
}


/**
 * Обработка команд
 */
void handle_packet()
{
    PORTA = 3;
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
    }
}

/**
 * Чтение пакета в блокируещем режиме и его обработка
 */
static void read_packet()
{
    PORTA = 2;
    int i;
    uart.read_sync(&pkt.cmd);
    uart.read_sync(&pkt.len);

    if ( pkt.len > PACKET_MAXLEN )
    {
        uint8_t dummy;
        for(i = 0; i < pkt.len; i++) uart.read_sync(&dummy);
        return;
    }

    for(i = 0; i < pkt.len; i++)
    {
        uart.read_sync(&pkt.data[i]);
    }

    handle_packet();
}

static uint8_t data[8];
static uint8_t offset = 0;

static void read_test_packet()
{
    uint8_t cmd;
    if ( uart.read_sync(&cmd) )
    {

        if ( cmd & (1 << 7) )
        {
            if ( offset < sizeof(data) )
            {
                data[offset++] = cmd & ~(1 << 7);
            }
        }
        else
        {
            uint8_t x;
            if ( offset > 0 )
            {
                x = data[--offset];
            }
            else
            {
                x = 0;
            }
            uart.write_sync(x);
        }

    }
}

int main()
{

    // Инициализация SPI в режиме мастера
    // PB1 управляет RESET-ом прошиваемого контроллера
    DDRB = makebits(MOSI_BIT, PB7, SS_BIT, PB1);
    SPCR = makebits(SPIE, SPE, MSTR, SPI2X, SPR1, SPR0);

    DDRA = 0xFF;
    PORTA = 1;//offset;

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
