#ifndef DEMO_UART_H
#define DEMO_UART_H

#include <tiny/system.h>
#include <tiny/io.h>
#include <armxx/bitband.h>
#include <tiny/uartbuf.h>

namespace demo
{

    class UART1
    {
    public:

        static inline USART_TypeDef* const port = USART1;

        static void init()
        {
            port->CR1 = tiny::makebits(USART_CR1_UE_Pos, USART_CR1_TE_Pos, USART_CR1_RE_Pos, USART_CR1_RXNEIE_Pos);
            port->CR2 = tiny::makebits(USART_CR2_STOP_Pos);
            port->CR3 = tiny::makebits(USART_CR3_ONEBIT_Pos);
            port->BRR = (52 << 4) | (1 << 0);
        }

        static void enableUDRE()
        {
            //port->CR1 = tiny::makebits(USART_CR1_UE_Pos, USART_CR1_TE_Pos, USART_CR1_RE_Pos, USART_CR1_RXNEIE_Pos, USART_CR1_TXEIE_Pos);
            arm::bitband_write(port->CR1, USART_CR1_TXEIE_Pos, true);
        }

        static void disableUDRE()
        {
            //port->CR1 = tiny::makebits(USART_CR1_UE_Pos, USART_CR1_TE_Pos, USART_CR1_RE_Pos, USART_CR1_RXNEIE_Pos);
            arm::bitband_write(port->CR1, USART_CR1_TXEIE_Pos, false);
        }

        static uint8_t read()
        {
            return port->DR;
        }

        static void write(uint8_t value)
        {
            port->DR = value;
        }

    };

    inline tiny::uartbuf<8, 8, UART1> uart1;

}

#endif // DEMO_UART_H
