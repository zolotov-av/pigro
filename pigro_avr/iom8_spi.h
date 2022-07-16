#ifndef __IOM16A_SPI_H_
#define __IOM16A_SPI_H_

/**
* Вывести байт в SPI
*/
extern char spi_putc(char data);

/**
* Прочитать байт из SPI
*/
extern char spi_getc(char *data);

/**
* Вывести строку в SPI
*/
extern char spi_puts(const char *s);

extern void spi_init();

#endif // __IOM16A_SPI_H_
