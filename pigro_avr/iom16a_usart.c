
#include <avr/io.h>
#include <avr/interrupt.h>

#include "iom16a_usart.h"

static char inp_buf[16];
static char out_buf[16];
static char inp_ofs_s = 0;
static char inp_ofs_e = 0;
static char inp_len = 0;
static char out_ofs_s = 0;
static char out_ofs_e = 0;
static char out_len = 0;


/**
* USART Receive Complete
*/
ISR(USART_RXC_vect)
{
	if ( inp_len < sizeof(inp_buf) )
	{
		inp_buf[inp_ofs_e++] = UDR;
		inp_ofs_e %= sizeof(inp_buf);
		inp_len++;
	}
}

/**
* USART Transmit Complete
*/
ISR(USART_TXC_vect)
{
}

/**
* USART Data Register Empty
*/
ISR(USART_UDRE_vect)
{
	if ( out_len > 0 )
	{
		UDR = out_buf[out_ofs_s++];
		out_ofs_s %= sizeof(out_buf);
		out_len--;
		if ( out_len == 0 )
		{
			UCSRB &= ~(1 << UDRIE);
		}
	}
}

char usart_putc(char data)
{
	cli();
	char status;
	if ( out_len < sizeof(out_buf) )
	{
		out_buf[out_ofs_e++] = data;
		out_ofs_e %= sizeof(out_buf);
		out_len++;
		status = 1;
		UCSRB |= (1 << UDRIE);
	}
	else status = 0;
	sei();
	return status;
}

char usart_getc(char *data)
{
	cli();
	char status;
	if ( inp_len > 0 )
	{
		*data = inp_buf[inp_ofs_s++];
		inp_ofs_s %= sizeof(inp_buf);
		inp_len--;
		status = 1;
	}
	else status = 0;
	sei();
	return status;
}

char usart_puts(const char *s)
{
	int r = 0;
	while ( *s )
	{
		if ( uart_putc(*s++) ) r++;
		else return r;
	}
	return r;
}
