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
#include <string.h>

#include <array>
#include <nano/exception.h>
#include <nano/serial.h>
#include <nano/IniReader.h>

#include "AVR.h"

constexpr uint8_t PKT_ACK = 1;
constexpr uint8_t PKT_NACK = 2;

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
std::string fuse_bits_s;

/**
* Нужно ли выводить дополнительный отладочный вывод или быть тихим?
*/
int verbose = 0;

static class PigroApp *papp = nullptr;

/**
 * Конверировать шестнадцатеричную цифру в число
 */
static uint8_t at_hex_digit(char ch)
{
    if ( ch >= '0' && ch <= '9' ) return ch - '0';
    if ( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
    if ( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
    throw nano::exception("wrong hex digit");
}

/**
 * Первести шестнадцатеричное число из строки в целочисленное значение
 */
static uint32_t at_hex_to_int(const char *s)
{
    if ( s[0] == '0' && (s[1] == 'x' || s[1] == 'X') ) s += 2;

    uint32_t r = 0;

    while ( *s )
    {
        char ch = *s++;
        uint8_t hex = at_hex_digit(ch);
        if ( hex > 0xF ) throw nano::exception("wrong hex digit");
        r = r * 0x10 + hex;
    }
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
    nano::serial *serial = nullptr;
    AVR_Info avr;

public:

    PigroApp(const char *path): serial (new nano::serial(path))
    {
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
            throw nano::exception("fail to send packet\n");
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
            throw nano::exception("packet to big: " + std::to_string(pkt->len) + "/" + std::to_string((PACKET_MAXLEN)));
        }
        for(int i = 0; i < pkt->len; i++)
        {
            pkt->data[i] = serial->read_sync();
        }
    }

    static void info(const char *msg)
    {
        if ( verbose )
        {
            printf("info: %s\n", msg);
        }
    }

    static void warn(const char *msg)
    {
        fprintf(stderr, "warn: %s\n", msg);
    }

    static void error(const char *msg)
    {
        fprintf(stderr, "error: %s\n", msg);
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
                    throw nano::exception("wrong protocol");
                nack_support = true;
                protoVersionMajor = pkt.data[0];
                protoVersionMinor = pkt.data[1];
                info("new protocol");
                return;
            }
            else
            {
                throw nano::exception("wrong protocol");
            }
        }

        nack_support = false;
        protoVersionMajor = 0;
        protoVersionMinor = 1;
        warn("old protocol? update firmware...");
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
        if ( pkt.cmd != 3 || pkt.len != 4 ) throw nano::exception("unexpected packet");

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
            throw nano::exception("isp_program_enable() failed");
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
    AVR::DeviceCode isp_chip_info()
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
        unsigned int r = cmd_isp_io(0xAC800000);
        bool status = ((r >> 16) & 0xFF) == 0xAC;
        if ( !status ) throw nano::exception("isp_chip_erase() error");
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

    /**
     * Загрузить байт прошивки в буфер страницы
     */
    void isp_load_memory_page(uint16_t addr, uint8_t byte)
    {
        uint8_t cmd = (addr & 1) ? 0x48 : 0x40;
        //uint16_t offset = (addr >> 1);
        uint16_t word_addr = (addr >> 1);
        uint32_t result = cmd_isp_io( (cmd << 24) | (word_addr << 8 ) | (byte & 0xFF) );
        uint8_t r = (result >> 16) & 0xFF;
        int status = (r == cmd);
        if ( !status ) throw nano::exception("isp_load_memory_page() error");
    }

    /**
     * Записать буфер страницы
     */
    int isp_write_memory_page(uint16_t page_addr)
    {
        uint8_t cmd = 0x4C;
        uint32_t result = cmd_isp_io( (cmd << 24) | (page_addr << 8 ) );
        uint8_t r = (result >> 16) & 0xFF;
        int status = (r == cmd);
        //sleep(1);
        return status;
    }

    void isp_check_firmware(const AVR_Data &pages)
    {
        auto signature = isp_chip_info();
        if ( signature != avr.signature )
        {
            warn("isp_check_firmware(): wrong chip signature");
        }

        for(const auto &[page_addr, page] : pages)
        {
            uint8_t counter = 0;
            const size_t size = page.data.size();
            for(size_t i = 0; i < size; i++)
            {
                const uint16_t addr = (page_addr * 2) + i;
                if ( counter == 0 ) printf("MEM[0x%04X]", addr);
                uint8_t byte = isp_read_memory(addr);
                printf("%s", (page.data[i] == byte ? "." : "*" ));
                if ( counter == 0x1F ) printf("\n");
                counter = (counter + 1) & 0x1F;
            }
        }
    }

    /**
     * Записать прошивку
     */
    void isp_write_firmware(const AVR_Data &pages)
    {
        auto signature = isp_chip_info();
        if ( signature != avr.signature )
        {
            throw nano::exception("isp_write_firmware() reject: wrong chip signature");
        }

        isp_chip_erase();
        for(const auto &[page_addr, page] : pages)
        {
            uint8_t counter = 0;
            const size_t size = page.data.size();
            for(size_t i = 0; i < size; i++)
            {
                const uint16_t addr = (page_addr * 2) + i;
                if ( counter == 0 ) printf("MEM[0x%04X]", addr);
                isp_load_memory_page((page_addr * 2) + i, page.data[i]);
                printf(".");
                if ( counter == 0x1F ) printf("\n");
                counter = (counter + 1) & 0x1F;

            }
            isp_write_memory_page(page_addr);
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

        uint8_t fuse_lo = cmd_isp_io(0x50000000) & 0xFF;
        uint8_t fuse_hi = cmd_isp_io(0x58080000) & 0xFF;
        uint8_t fuse_ex = cmd_isp_io(0x50080000) & 0xFF;

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
        auto fuse_bits = at_hex_to_int(fuse_bits_s.c_str());

        if ( verbose )
        {
            printf("write device's low fuse bits (0x%02X)\n", fuse_bits);
        }

        if ( fuse_bits > 0xFF )
        {
            printf("wrong fuse bits\n");
            return 0;
        }

        unsigned int r = cmd_isp_io(0xACA00000 + (fuse_bits & 0xFF));
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
        auto fuse_bits = at_hex_to_int(fuse_bits_s.c_str());

        if ( verbose )
        {
            printf("write device's high fuse bits (0x%02X)\n", fuse_bits);
        }

        if ( fuse_bits > 0xFF )
        {
            printf("wrong fuse bits\n");
            return 0;
        }

        unsigned int r = cmd_isp_io(0xACA80000 + (fuse_bits & 0xFF));
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
        auto fuse_bits = at_hex_to_int(fuse_bits_s.c_str());

        if ( verbose )
        {
            printf("write device's extended fuse bits (0x%02X)\n", fuse_bits);
        }

        if ( fuse_bits > 0xFF )
        {
            printf("wrong fuse bits\n");
            return 0;
        }

        unsigned int r = cmd_isp_io(0xACA40000 + (fuse_bits & 0xFF));
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

    AVR_Data readHEX()
    {
        nano::IniReader ini("pigro.ini");

        if ( const std::string output = ini.value("main", "output", "quiet"); output == "verbose" )
        {
            verbose = true;
        }

        avr.signature = {0x1E, 0x94, 0x03};
        avr.page_word_size = 64;
        avr.page_count = 128;

        std::string device = ini.value("main", "device");
        if ( device.empty() )
        {
            throw nano::exception("specify device");
        }

        if ( auto dev = AVR::findDeviceByName(device); dev.has_value() )
        {
            avr = *dev;
        }

        if ( verbose )
        {
            std::cout << "device: " << device << "\n";
            std::cout << "page_size: " << int(avr.page_word_size) << "\n";
            std::cout << "page_count: " << int(avr.page_count) << "\n";
            std::cout << "flash_size: " << ((avr.flash_size()+1023) / 1024) << "k\n";
        }

        auto hexfname = ini.value("main", "hex", fname);

        auto pages = avr_load_from_hex(avr, hexfname);
        printf("page usages: %ld / %d\n", pages.size(), avr.page_count);
        return pages;
    }

    /**
     * Действие - проверить прошивку в устройстве
     */
    int action_check()
    {
        AVR_Data pages = readHEX();
        isp_check_firmware(pages);
        return 0;
    }

    /**
     * Действие - записать прошивку в устройство
     */
    int action_write()
    {
        AVR_Data pages = readHEX();
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
        default: throw nano::exception("Victory!");
        }
    }

};

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
	
    if ( argc > 2 )
	{
        fuse_bits_s = argv[2];
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
    catch (const nano::exception &e)
    {
        std::cerr << "[nano::exception] " << e.message() << std::endl;
        return 1;
    }
    catch(const std::exception &e)
    {
        std::cerr << "[std::exception] " << e.what() << std::endl;
        return 1;
    }
}
