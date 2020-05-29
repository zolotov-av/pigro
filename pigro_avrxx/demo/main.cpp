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

int main()
{

    // TODO: UARTConfig -> UART::setMode()
    UARTConfig config;
    config.setCharacterSize(8);
    config.apply();

    UART::enableTransmitter();
    UART::enableReceiver();
    UART::enableRXC();
    UART::enableTXC();
    UART::enableUDRE();
    UART::setBaudRate(UART_BAUD_K);
    /*
    // Настройка UART
    UCSRA = makebits();
    UCSRB = makebits(RXEN, TXEN, RXCIE, TXCIE, UDRIE);
    UCSRC = makebits(URSEL, UCSZ0, UCSZ1);

    // Настройка частоты UART
    UART::setBaudRate(UART_BAUD_K);
*/
    avr::interrupt_enable();
    avr::sleep_loop();
}
