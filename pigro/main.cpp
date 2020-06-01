//
// Программатор микроконтроллеров AVR.
// 
// Изначально был написан на Raspberry Pi и для Raspberry Pi и использовал
// шину GPIO на Raspberry Pi, к которой непосредственно подключался
// программируемый контроллер. Эта версия программатора использует внешений
// самодельный переходник USB <-> SPI и может запускаться теоретически на
// любой linux-системе.
//
// (c) Alex V. Zolotov <zolotov-alex@shamangrad.net>, 2013
//    Fill free to copy, to compile, to use, to redistribute etc on your own risk.
//

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>  /* Объявления стандартных функций UNIX */
#include <fcntl.h>   /* Объявления управления файлами */
#include <errno.h>   /* Объявления кодов ошибок */
#include <termios.h> /* Объявления управления POSIX-терминалом */
#include <string.h>

#include <array>

#include "serial.h"
#include "IntelHEX.h"

constexpr uint8_t PKT_ACK = 1;
constexpr uint8_t PKT_NACK = 2;

int at_flush();

enum PigroAction {
	AT_ACT_INFO,
	AT_ACT_CHECK,
	AT_ACT_WRITE,
	AT_ACT_ERASE,
	AT_ACT_READ_FUSE,
	AT_ACT_WRITE_FUSE_LO,
	AT_ACT_WRITE_FUSE_HI,
	AT_ACT_WRITE_FUSE_EX,
	AT_ACT_ADC
};

constexpr auto PACKET_MAXLEN = 6;

struct packet_t
{
	unsigned char cmd;
	unsigned char len;
	unsigned char data[PACKET_MAXLEN];
};

/**
* Имя hex-файла
*/
const char *fname;

/**
* FUSE-биты
*/
unsigned int fuse_bits;

/**
* Нужно ли выводить дополнительный отладочный вывод или быть тихим?
*/
int verbose = 0;

static class PigroApp *papp = nullptr;

int send_packet(const packet_t *pkt);
int recv_packet(packet_t *pkt);

unsigned int at_io(unsigned int cmd);
unsigned int cmd_isp_io(unsigned int cmd);
int at_chip_erase();

/**
* Прочитать байт прошивки из устройства
*/
unsigned char at_read_memory(unsigned int addr)
{
	unsigned int cmd = (addr & 1) ? 0x28 : 0x20;
	unsigned int offset = (addr >> 1) & 0xFFFF;
	unsigned int byte = at_io( (cmd << 24) | (offset << 8 ) ) & 0xFF;
	return byte;
}

/**
* Адрес активной страницы
*/
static unsigned int active_page = 0;

/**
* Флаг грязной страницы
* 
* Указывает что в буфер текущей страницы были записаны
* данные и требуется запись во флеш.
*/
static unsigned int dirty_page = 0;

/**
* Записать байт прошивки в устройство
* NOTE: байт сначала записывается в специальный буфер, фиксация
* данных происходит в функции at_flush(). Функция at_write_memory()
* сама переодически вызывает at_flush() поэтому нет необходимости
* часто вызывать at_flush() и необходимо только в конце, чтобы
* убедиться что последние данные будут записаны в устройство
*/
int at_write_memory(unsigned int addr, unsigned char byte)
{
	unsigned int cmd = (addr & 1) ? 0x48 : 0x40;
	unsigned int offset = (addr >> 1) & 0xFFFF;
	unsigned int page = (addr >> 1) & 0xFFF0;
	unsigned int x = 0;
	if ( page != active_page )
	{
		at_flush();
		active_page = page;
	}
	dirty_page = 1;
	unsigned int result = at_io(x = (cmd << 24) | (offset << 8 ) | (byte & 0xFF) );
	unsigned int r = (result >> 16) & 0xFF;
	int status = (r == cmd);
	if ( verbose )
	{
		printf(status ? "." : "*");
		//printf("[%04X]=%02X%s ", offset, byte, (status ? "+" : "-"));
	}
	return status;
}

/**
* Завершить запись данных
*/
int at_flush()
{
	if ( ! dirty_page )
	{
		return 1;
	}
	
	unsigned int cmd = 0x4C;
	unsigned int offset = active_page & 0xFFF0;
	unsigned int x = 0;
	unsigned int result = at_io(x = (cmd << 24) | (offset << 8 ) );
	unsigned int r = (result >> 16) & 0xFF;
	int status = (r == cmd);
	dirty_page = 0;
	if ( verbose )
	{
		printf("FLUSH[%04X]%s\n", offset, (status ? "+" : "-"));
	}
	return status;
}

/**
* выдать сообщение об ошибке в hex-файле
*/
void at_wrong_file()
{
	printf("wrong hex-file\n");
}

/**
* Конверировать шестнадцатеричную цифру в число
*/
unsigned char at_hex_digit(char ch)
{
	if ( ch >= '0' && ch <= '9' ) return ch - '0';
	if ( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
	if ( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
	// TODO somethink...
	return 0;
}

/**
* Первести шестнадцатеричное число из строки в целочисленное значение
*/
unsigned int at_hex_to_int(const char *s)
{
	unsigned int r = 0;
	while ( *s )
	{
		char ch = *s++;
		unsigned int hex = 0x10;
		if ( ch >= '0' && ch <= '9' ) hex = ch - '0';
		else if ( ch >= 'A' && ch <= 'F' ) hex = ch - 'A' + 10;
		else if ( ch >= 'a' && ch <= 'f' ) hex = ch - 'a' + 10;
		else hex = 0x10;
		if ( hex > 0xF ) return r;
		r = r * 0x10 + hex;
	}
	return r;
}

/**
* Прочитать байт
*/
unsigned char at_hex_get_byte(const char *line, int i)
{
	int offset = i * 2 + 1;
	// TODO index limit checks
	return at_hex_digit(line[offset]) * 16 + at_hex_digit(line[offset+1]);
}

/**
* Прочитать слово (два байта)
*/
unsigned int at_hex_get_word(const char *line, int i)
{
	return at_hex_get_byte(line, i) * 256 + at_hex_get_byte(line, i+1);
}

/**
* Прочитать байт данных (читает из секции данных)
*/
unsigned char at_hex_get_data(const char *line, int i)
{
	return at_hex_get_byte(line, i + 4);
}

/**
* Сверить прошивку с данными из файла
*/
int at_check_firmware(const char *fname)
{
	FILE *f = fopen(fname, "r");
	if ( f )
	{
		int lineno = 0;
		int result = 1;
		int bytes = 0;
		while ( 1 )
		{
			char line[1024];
			const char *s = fgets(line, sizeof(line), f);
			if ( s == NULL ) break;
			lineno++;
			if ( line[0] != ':' )
			{
				at_wrong_file();
				break;
			}
			unsigned char len = at_hex_get_byte(line, 0);
			unsigned int addr = at_hex_get_word(line, 1);
			unsigned char type = at_hex_get_byte(line, 3);
            //unsigned char cc = at_hex_get_byte(line, 4 + len);
			if ( verbose ) printf("[%04X]", addr);
			if ( type == 0 )
			{
				int i;
				for(i = 0; i < len; i++)
				{
					bytes ++;
					unsigned char fbyte = at_hex_get_data(line, i);
					unsigned char dbyte = at_read_memory(addr + i);
					int r = (fbyte == dbyte);
					result = result && r;
					if ( verbose )
					{
						printf(r ? "." : "*");
					}
				}
				if ( verbose ) printf("\n");
			}
			if ( type == 1 )
			{
				if ( verbose ) printf("end of hex-file\n");
				break;
			}
		}
		fclose(f);
		return result;
	}
	return 0;
}

/**
* Записать прошивку в устройство
*/
int at_write_firmware(const char *fname)
{
	FILE *f = fopen(fname, "r");
	if ( f )
	{
		int lineno = 0;
		int result = 1;
		int bytes = 0;
		while ( 1 )
		{
			char line[1024];
			const char *s = fgets(line, sizeof(line), f);
			if ( s == NULL ) break;
			lineno++;
			if ( line[0] != ':' )
			{
				at_wrong_file();
				break;
			}
			unsigned char len = at_hex_get_byte(line, 0);
			unsigned int addr = at_hex_get_word(line, 1);
			unsigned char type = at_hex_get_byte(line, 3);
            //unsigned char cc = at_hex_get_byte(line, 4 + len);
			if ( type == 0 )
			{
				int i;
				for(i = 0; i < len; i++)
				{
					bytes++;
					unsigned char fbyte = at_hex_get_data(line, i);
					int r = at_write_memory(addr + i, fbyte);
					result = result && r;
				}
			}
			if ( type == 1 )
			{
				at_flush();
				if ( verbose ) printf("end of hex-file\n");
				break;
			}
		}
		at_flush();
		fclose(f);
		if ( verbose )
		{
            const char *st = result ? "ok" : "fail";
			printf("memory write: %s, bytes: %d\n", st, bytes);
		}
		return result;
	}
	return 0;
}

/**
* Действие - сверить прошивку в устрействе с файлом
*/
int at_act_check()
{
	int r = at_check_firmware(fname);
	if ( r ) printf("firmware is same\n");
	else printf("firmware differ\n");
	return r;
}

/**
* Отобразить подсказку
*/
int help()
{
	printf("pigro :action: :filename: :verbose|quiet:\n");
	printf("  action:\n");
	printf("    info  - read chip info\n");
	printf("    check - read file and compare with device\n");
	printf("    write - read file and write to device\n");
	printf("    erase - just erase chip\n");
	printf("    rfuse - read fuses\n");
	printf("    wfuse_lo - write low fuse bits\n");
	printf("    wfuse_hi - write high fuse bits\n");
    printf("    wfuse_ex - write extended fuse bits\n");
	return 0;
}

class PigroApp
{
private:

    bool nack_support = false;
    uint8_t protoVersionMajor = 0;
    uint8_t protoVersionMinor = 0;
    pigro::serial *serial = nullptr;

public:

    PigroApp(const char *path): serial (new pigro::serial(path))
    {
        if ( serial == nullptr )
        {
            throw pigro::exception("fail to create pigro::serial");
        }
    }

    PigroApp(pigro::serial *tty): serial(tty)
    {
        if ( serial == nullptr )
        {
            throw pigro::exception("serial = nullptr");
        }
    }

    ~PigroApp()
    {
        delete serial;
    }

    /**
     * Отправить пакет данных
     */
    bool send_packet(const packet_t *pkt)
    {
        ssize_t r = serial->write(pkt, pkt->len + 2);
        if ( r != pkt->len + 2 )
        {
            // TODO обработка ошибок
            throw pigro::exception("fail to send packet\n");
        }

        if ( nack_support )
        {
            return serial->read_sync() == PKT_ACK;
        }

        return true;
    }

    void recv_packet(packet_t *pkt)
    {
        pkt->cmd = serial->read_sync();
        pkt->len = serial->read_sync();
        if ( pkt->len >= PACKET_MAXLEN )
        {
            throw pigro::exception("packet to big: " + std::to_string(pkt->len) + "/" + std::to_string((PACKET_MAXLEN)));
        }
        for(int i = 0; i < pkt->len; i++)
        {
            pkt->data[i] = serial->read_sync();
        }
    }


    static void info(const char *msg)
    {
        printf("info: %s\n", msg);
    }

    void checkVersion()
    {
        packet_t pkt;
        pkt.cmd = 1;
        pkt.len = 2;
        pkt.data[0] = 0;
        pkt.data[1] = 0;
        send_packet(&pkt);

        if ( serial->wait() )
        {
            uint8_t ack;
            serial->read(&ack, sizeof(ack));
            if ( ack == PKT_ACK )
            {
                recv_packet(&pkt);
                if ( pkt.len != 2 )
                    throw pigro::exception("wrong protocol");
                nack_support = true;
                protoVersionMajor = pkt.data[0];
                protoVersionMinor = pkt.data[1];
                info("new protocol");
                return;
            }
            else
            {
                throw pigro::exception("wrong protocol");
            }
        }

        nack_support = false;
        protoVersionMajor = 0;
        protoVersionMinor = 1;
        info("old protocol");

    }

    void dumpProtoVersion()
    {
        printf("proto version: %d.%d\n", protoVersionMajor, protoVersionMinor);
    }

    /**
     * @brief set PORTA
     * @param value
     * @return
     */
    void cmd_seta(uint8_t value)
    {
        packet_t pkt;
        pkt.cmd = 1;
        pkt.len = 1;
        pkt.data[0] = value;
        send_packet(&pkt);
    }

    /**
     * @brief Управление линией RESET программируемого контроллера
     * @param value
     * @return
     */
    void cmd_isp_reset(bool value)
    {
        packet_t pkt;
        pkt.cmd = 2;
        pkt.len = 1;
        pkt.data[0] = value ? 1 : 0;
        send_packet(&pkt);
    }

    /**
     * Отправить команду программирования (ISP)
     */
    unsigned int cmd_isp_io(unsigned int cmd)
    {
        packet_t pkt;
        pkt.cmd = 3;
        pkt.len = 4;

        for(int i = 0; i < 4; i++)
        {
            unsigned char byte = cmd >> 24;
            pkt.data[i] = byte;
            cmd <<= 8;
        }

        send_packet(&pkt);

        recv_packet(&pkt);
        if ( pkt.cmd != 3 || pkt.len != 4 ) throw pigro::exception("unexpected packet");

        unsigned int result = 0;
        for(int i = 0; i < 4; i++)
        {
            unsigned char byte = pkt.data[i];
            result = result * 256 + byte;
        }

        return result;
    }


    /**
     * @brief Подать сигнал RESET и командду "Programming Enable"
     * @return
     */
    int isp_program_enable()
    {
        cmd_isp_reset(0);
        cmd_isp_reset(1);
        cmd_isp_reset(0);

        unsigned int r = cmd_isp_io(0xAC530000);
        int status = (r & 0xFF00) == 0x5300;
        if ( verbose || !status )
        {
            const char *s = status ? "ok" : "fault";
            printf("at_program_enable(): %s 0x%08x\n", s, r);
        }
        if ( !status )
        {
            throw pigro::exception("isp_program_enable() failed");
        }
        return status;
    }

    /**
     * Подать команду "Read Device Code"
     *
     *
     * Лучший способ проверить связь с чипом в режиме программирования.
     * Эта команда возвращает код который индентфицирует модель чипа.
     */
    pigro::AVR_Signature isp_chip_info()
    {
        uint8_t b000 = cmd_isp_io(0x30000000) & 0xFF;
        uint8_t b001 = cmd_isp_io(0x30000100) & 0xFF;
        uint8_t b002 = cmd_isp_io(0x30000200) & 0xFF;

        return {b000, b001, b002};
    }

    /**
     * Команда "Chip Erase"
     *
     * Отправить команду стирания чипа, чип уже должен быть переведен в режим
     * программирования.
     */
    void isp_chip_erase()
    {
        if ( verbose )
        {
            info("erase device's firmware");
        }
        unsigned int r = at_io(0xAC800000);
        bool status = ((r >> 16) & 0xFF) == 0xAC;
        if ( !status ) throw pigro::exception("isp_chip_erase() error");
    }

    /**
     * Прочитать байт прошивки из устройства
     */
    uint8_t isp_read_memory(uint16_t addr)
    {
        uint8_t cmd = (addr & 1) ? 0x28 : 0x20;
        uint16_t offset = (addr >> 1);
        return cmd_isp_io( (cmd << 24) | (offset << 8 ) ) & 0xFF;
    }

    void isp_check_firmware(const pigro::AVR_Data &pages)
    {
        for(const auto &[page_addr, page] : pages)
        {
            printf("PAGE[0x%04X]", page_addr);
            const size_t size = page.data.size();
            for(size_t i = 0; i < size; i++)
            {
                uint8_t byte = isp_read_memory((page_addr * 2) + i);
                printf("%s", (page.data[i] == byte ? "." : "*" ));
                fflush(stdout);
            }
            printf(" %ld\n", size);
        }
    }

    /**
     * Записать прошивку
     */
    void isp_write_firmware(const pigro::AVR_Data &pages)
    {
        for(const auto &[page_addr, page] : pages)
        {

        }
    }


    /**
     * @brief Действие - вывести информацию об устройстве
     * @return
     */
    int action_info()
    {
        auto info = isp_chip_info();
        printf("chip signature: 0x%02X, 0x%02X, 0x%02X\n", info[0], info[1], info[2]);
        return 1;
    }

    /**
     * Действие - прочитать биты fuse
     */
    int action_read_fuse()
    {
        if ( verbose )
        {
            info("read device's fuses");
        }

        uint8_t fuse_lo = at_io(0x50000000) & 0xFF;
        uint8_t fuse_hi = at_io(0x58080000) & 0xFF;
        uint8_t fuse_ex = at_io(0x50080000) & 0xFF;

        printf("fuse[lo]: 0x%02X\n", fuse_lo);
        printf("fuse[hi]: 0x%02X\n", fuse_hi);
        printf("fuse[ex]: 0x%02X\n", fuse_ex);

        return 1;
    }

    /**
     * Действие - записать младшие биты fuse
     */
    int action_write_fuse_lo()
    {
        if ( verbose )
        {
            printf("write device's low fuse bits (0x%02X)\n", fuse_bits);
        }

        if ( fuse_bits > 0xFF )
        {
            printf("wrong fuse bits\n");
            return 0;
        }

        unsigned int r = at_io(0xACA00000 + (fuse_bits & 0xFF));
        int ok = ((r >> 16) & 0xFF) == 0xAC;

        if ( verbose )
        {
            const char *status = ok ? "[ ok ]" : "[ fail ]";
            printf("write device's low fuse bits %s\n", status);
        }

        return ok;
    }

    /**
     * Действие - записать старшие биты fuse
     */
    int action_write_fuse_hi()
    {
        if ( verbose )
        {
            printf("write device's high fuse bits (0x%02X)\n", fuse_bits);
        }
        if ( fuse_bits > 0xFF )
        {
            printf("wrong fuse bits\n");
            return 0;
        }

        unsigned int r = at_io(0xACA80000 + (fuse_bits & 0xFF));
        int ok = ((r >> 16) & 0xFF) == 0xAC;
        if ( verbose )
        {
            const char *status = ok ? "[ ok ]" : "[ fail ]";
            printf("write device's high fuse bits %s\n", status);
        }

        return ok;
    }

    /**
     * Действие - записать расширеные биты fuse
     */
    int action_write_fuse_ex()
    {
        if ( verbose )
        {
            printf("write device's extended fuse bits (0x%02X)\n", fuse_bits);
        }
        if ( fuse_bits > 0xFF )
        {
            printf("wrong fuse bits\n");
            return 0;
        }

        unsigned int r = at_io(0xACA40000 + (fuse_bits & 0xFF));
        int ok = ((r >> 16) & 0xFF) == 0xAC;
        if ( verbose )
        {
            const char *status = ok ? "[ ok ]" : "[ fail ]";
            printf("write device's extended fuse bits %s\n", status);
        }

        return ok;
    }

    /**
     * Действие - стереть чип
     */
    int action_erase()
    {
        isp_chip_erase();
        return 1;
    }

    pigro::AVR_Data readHEX()
    {
        pigro::IntelHEX hex;
        hex.open(fname);

        pigro::AVR_Info avr;
        avr.page_word_size = 64;
        avr.page_count = 128;
        auto pages = hex.split_pages(avr);
        printf("page usages: %ld / %d\n", pages.size(), avr.page_count);
        return pages;
    }

    /**
     * Действие - проверить прошивку в устройстве
     */
    int action_check()
    {
        pigro::AVR_Data pages = readHEX();
        isp_check_firmware(pages);
        return 0;
    }

    /**
     * Действие - записать прошивку в устройство
     */
    int action_write()
    {
        pigro::AVR_Data pages = readHEX();
        isp_write_firmware(pages);
        return 0;
    }

    /**
    * Запус команды
    */
    int execute(PigroAction action)
    {
        switch ( action )
        {
        case AT_ACT_INFO: return action_info();
        case AT_ACT_CHECK: return action_check();
        case AT_ACT_WRITE: return action_write();
        case AT_ACT_ERASE: return action_erase();
        case AT_ACT_READ_FUSE: return action_read_fuse();
        case AT_ACT_WRITE_FUSE_LO: return action_write_fuse_lo();
        case AT_ACT_WRITE_FUSE_HI: return action_write_fuse_hi();
        case AT_ACT_WRITE_FUSE_EX: return action_write_fuse_ex();
        default: throw pigro::exception("Victory!");
        }
    }

};

/**
* Отправить пакет данных
*/
int send_packet(const packet_t *pkt)
{
    papp->send_packet(pkt);
    return 1;
}

/**
* Прочитать пакет данных
*/
int recv_packet(packet_t *pkt)
{
    papp->recv_packet(pkt);
    return 1;
}

/**
* Отправить комманду микроконтроллеру и прочтать ответ
* Все комманды размером 4 байта
*/
unsigned int at_io(unsigned int cmd)
{
    return papp->cmd_isp_io(cmd);
}

/**
 * Отправить команду программирования (ISP)
 */
unsigned int cmd_isp_io(unsigned int cmd)
{
    return papp->cmd_isp_io(cmd);
}

int at_chip_erase()
{
    papp->isp_chip_erase();
    return true;
}

int real_main(int argc, char *argv[])
{
	if ( argc <= 1 ) return help();
	
    PigroAction action;

	if ( strcmp(argv[1], "info") == 0 ) action = AT_ACT_INFO;
	else if ( strcmp(argv[1], "check") == 0 ) action = AT_ACT_CHECK;
	else if ( strcmp(argv[1], "write") == 0 ) action = AT_ACT_WRITE;
	else if ( strcmp(argv[1], "erase") == 0 ) action = AT_ACT_ERASE;
	else if ( strcmp(argv[1], "rfuse") == 0 ) action = AT_ACT_READ_FUSE;
	else if ( strcmp(argv[1], "wfuse_lo") == 0 ) action = AT_ACT_WRITE_FUSE_LO;
	else if ( strcmp(argv[1], "wfuse_hi") == 0 ) action = AT_ACT_WRITE_FUSE_HI;
    else if ( strcmp(argv[1], "wfuse_ex") == 0 ) action = AT_ACT_WRITE_FUSE_EX;
	else return help();
	
	fuse_bits = 0x100;
	if ( argc > 2 )
	{
		fuse_bits = at_hex_to_int(argv[2]);
	}
	
	fname = argc > 2 ? argv[2] : "firmware.hex";
	verbose = argc > 3 && strcmp(argv[3], "verbose") == 0;
	if ( verbose )
	{
		printf("firmware file: %s\n", fname);
	}

    PigroApp app("/dev/ttyUSB0");
    app.checkVersion();
    app.dumpProtoVersion();
    papp = &app;

    app.isp_program_enable();
    app.execute(action);
    app.cmd_isp_reset(1);

    return 0;
}

int main(int argc, char *argv[])
{
    try
    {
        return real_main(argc, argv);
    }
    catch (const pigro::exception &e)
    {
        std::cerr << "error: " << e.message() << std::endl;
        return 1;
    }
}
