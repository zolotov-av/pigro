#include <avrxx.h>
#include <spibuf.h>
#include <uartbuf.h>
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


ISR(SPI_STC_vect)
{

}

volatile uint8_t test;

int main()
{

    const bitfield<uint8_t> upm( UCSRC, UPM0, 2 );

    //if ( upm.value() == 3 ) test = 1;


    // Настройка UART
    ioreg(UCSRA).set(0);
    ioreg reg(UCSRB);
    reg.clear();
    UART().enableReceiver();
    UART().disableTransmitter();
    reg.setPin(RXCIE, true);
    reg.setPin(TXCIE, true);
    reg.setPin(UDRIE, true);
    ioreg(UCSRC).set( (1<<URSEL)|(1<<UCSZ0)|(1<<UCSZ1) );

    upm.set(2);

    // Настройка частоты UART
    UART().setBaudRate(UART_BAUD_K);


    avr::interrupt_enable();
    avr::sleep_loop();
}
