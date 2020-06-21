#include "uart.h"

void USART1_IRQHandler()
{
    const auto SR = demo::UART1::port->SR;

    if ( SR & USART_SR_RXNE )
    {
        demo::uart1.handle_rxc_isr();
    }

    if ( SR & USART_SR_TXE )
    {
        demo::uart1.handle_udre_isr();
    }
}
