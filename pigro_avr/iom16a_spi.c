
#include <avr/io.h>
#include <avr/interrupt.h>

#include "iom16a_spi.h"

static char inp_buf[16];
static char out_buf[16];
static char inp_ofs_s;
static char inp_ofs_e;
static char inp_len;
static char out_ofs_s;
static char out_ofs_e;
static char out_len;
static char out_ready;

/**
* Прерываение SPI
*/
ISR(SPISTC_vect)
{
	char data1 = SPDR;
	if ( out_len > 0 )
	{
		SPDR = out_buf[out_ofs_s++];
		out_ofs_s %= sizeof(out_buf);
		out_len--;
	}
	else
	{
		out_ready = 1;
	}
	if ( inp_len < sizeof(inp_buf) )
	{
		inp_buf[inp_ofs_e++] = data1;
		inp_ofs_e %= sizeof(inp_buf);
		inp_len++;
	}
}

/**
* Вывести байт в SPI
*/
char spi_putc(char data)
{
	cli();
	char status;
	if ( out_len < sizeof(out_buf) )
	{
		if ( out_ready )
		{
			SPDR = data;
			out_ready = 0;
			status = 1;
		}
		else
		{
			out_buf[out_ofs_e++] = data;
			out_ofs_e %= sizeof(out_buf);
			out_len++;
			status = 1;
		}
	}
	else status = 0;
	sei();
	return status;
}

/**
* Прочитать байт из SPI
*/
char spi_getc(char *data)
{
	cli();
	char status;
	if ( inp_len > 0 )
	{
		data[0] = inp_buf[inp_ofs_s++];
		inp_ofs_s %= sizeof(inp_buf);
		inp_len--;
		status = 1;
	}
	else status = 0;
	sei();
	return status;
}

/**
* Вывести строку в SPI
*/
char spi_puts(const char *s)
{
	int r = 0;
	while ( *s )
	{
		if ( spi_putc(*s++) ) r++;
		else return r;
	}
	return r;
}

void spi_init()
{
	inp_ofs_s = 0;
	inp_ofs_e = 0;
	inp_len = 0;
	out_ofs_s = 0;
	out_ofs_e = 0;
	out_len = 0;
	out_ready = 1;
}
