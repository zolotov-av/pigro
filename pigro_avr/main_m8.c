
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "iom8_usart.h"
#include "iom8_spi.h"

#define PACKET_MAXLEN 6

// 51 at 8MHz = 9600
#define UART_BAUD_K 51

typedef struct
{
	unsigned char cmd;
	unsigned char len;
	unsigned char data[PACKET_MAXLEN];
} packet_t;

/**
* Чтение из UART в блокируещем режиме
*/
char usart_getc_sync()
{
	// костыль
	static char uart_data;

	while ( ! usart_getc(&uart_data) )
	{
		set_sleep_mode(0);
		sleep_enable();
		sleep_cpu();
	}
	return uart_data;
}

/**
* Чтение из SPI в блокируещем режиме
*/
char spi_getc_sync()
{
	// Костыль
	static char spi_data;
	
	while ( ! spi_getc(&spi_data) )
	{
		set_sleep_mode(0);
		sleep_enable();
		sleep_cpu();
	}
	return spi_data;
}

/**
* Пакет данных
*/
packet_t pkt;

/**
* Отправить пакет сформированный в переменной ptk
*/
void send_packet()
{
	usart_putc(pkt.cmd);
	usart_putc(pkt.len);
	int i;
	for(i = 0; i < pkt.len; i++)
	{
		usart_putc(pkt.data[i]);
	}
}

/**
* Обработка команды seta
*/
void cmd_seta()
{
	//if ( pkt.len == 1 ) PORTA = pkt.data[0];
}

/**
* Обработка команды isp_reset
*/
void cmd_isp_reset()
{
	if ( pkt.len == 1 )
	{
		if ( pkt.data[0] ) PORTB |= (1<<PB1);
		else PORTB &= ~(1<<PB1);
	}
}

/**
* Обработка команды isp_io
*/
void cmd_isp_io()
{
	int i;
	if ( pkt.len == 4 )
	{
		for(i = 0; i < 4; i++)
		{
			spi_putc(pkt.data[i]);
		}
		for(i = 0; i < 4; i++)
		{
			pkt.data[i] = spi_getc_sync();
		}
		send_packet();
	}
}

static char adc_en = 0;

ISR(ADC_vect)
{
	if ( adc_en )
	{
		pkt.cmd = 4;
		pkt.len = 2;
		pkt.data[0] = ADCL;
		pkt.data[1] = ADCH;
		send_packet();
	}
	adc_en = 0;
	ADCSRA &= ~(1 << ADEN);
}

/**
* Обработчик команды ADC
*/
void cmd_adc()
{
	if ( pkt.len == 1 )
	{
		ADMUX = pkt.data[0] & ~(1 << ADLAR);
	}
	adc_en = 1;
	ADCSRA |= (1 << ADEN);
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
		cmd_adc();
		return;
	}
}

/**
* Чтение пакета в блокируещем режиме и его обработка
*/
void read_packet()
{
	int i;
	pkt.cmd = usart_getc_sync();
	pkt.len = usart_getc_sync();
	if ( pkt.len > PACKET_MAXLEN )
	{
		for(i = 0; i < pkt.len; i++) usart_getc_sync();
	}
	else
	{
		for(i = 0; i < pkt.len; i++)
		{	
			pkt.data[i] = usart_getc_sync();
		}
		handle_packet();
	}
}

int main()
{
	// Инициализация SPI в режиме мастера
	// PB1 управляет RESET-ом прошиваемого контроллера
	DDRB = (1 << PB3) | (1 << PB5) | (1 << PB2) | (1<<PB1);
	SPCR = (1 << SPIE) | (1 << SPE) | (1 << MSTR) | (0 << SPI2X) | (1 << SPR1) | (1 << SPR0);
	spi_init();
	
	// Настройка UART
	UCSRA = 0;
	UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE)|(1<<TXCIE)|(1<<UDRIE);
	UCSRC = (1<<URSEL)|(1<<UCSZ0)|(1<<UCSZ1);
	
	// Настройка частоты UART
	UBRRH = (UART_BAUD_K / 256) & ~(1 <<URSEL);
	UBRRL = UART_BAUD_K % 256;
	
	// Настройка прочих портов
	//DDRA = 0x00;
	//DDRC = 0xFF;
	//PORTC = 0xFF;
	
	ADMUX = (1 << ADLAR);
	ADCSRA = (0 << ADEN) | (0 << ADSC) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0);
	SFIOR = 0;
	
	// В бесконечом цикле читаем и обрабатываем пакеты
	while ( 1 )
	{
		read_packet();
	}
	
	return 0;
}
