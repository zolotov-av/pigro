#include <avrxx/io.h>
#include <avrxx/spibuf.h>
#include <avrxx/uartbuf.h>
#include <avr/io.h>
#include <avr/interrupt.h>

using namespace avr;

// 51 at 8MHz = 9600
constexpr auto UART_BAUD_K = 51;

uartbuf<8, 8> uart {};

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

static uint8_t data[8];
static uint8_t offset = 0;

static void read_packet()
{
    uart.wait();
    uint8_t cmd;
    if ( uart.read(&cmd) )
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
            uart.write(x);
        }

        PORTA = offset;

    }
}

int main()
{
    DDRA = 0xFF;
    PORTA = offset;

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
