#include <avr/interrupt.h>
#include <tiny/system.h>

#include "PigroService.h"

// 51 at 8MHz = 9600
//constexpr auto UART_BAUD_K = 51;

// 47 at 7.3728 MHz = 9600
constexpr auto UART_BAUD_K = 47;

// 103 at 16MHz = 9600
//constexpr auto UART_BAUD_K = 103;

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

int main()
{

    // Инициализация SPI в режиме мастера
    // PB1 управляет RESET-ом прошиваемого контроллера
    DDRB = tiny::makebits(MOSI_BIT, PB7, SS_BIT, PB1);
    SPCR = tiny::makebits(SPIE, SPE, MSTR, SPI2X, SPR1, SPR0);

    DDRA = tiny::makebits(PA0, PA1, PA2, PA3, PA4, /*PA5,*/ PA6, PA7);
    PORTA = JTAG_DEFAULT_STATE;

    //DDRA = 0xFF;
    //PORTA = 0xFF;

    avr::pin(MCUCR, SE).set(true);

    // Настройка UART
    avr::UART::init();
    avr::UART::setBaudRate(UART_BAUD_K);

    tiny::interrupt_enable();

    PigroService::run();
}
