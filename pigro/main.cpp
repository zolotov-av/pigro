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
    AT_ACT_WRITE_FUSE,
	AT_ACT_WRITE_FUSE_LO,
	AT_ACT_WRITE_FUSE_HI,
	AT_ACT_WRITE_FUSE_EX,
    AT_ACT_ADC,
    AT_ACT_ARM
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
static uint32_t parse_hexint(const char *s)
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

static uint8_t parse_fuse(const std::string &s, const char *type)
{
    uint32_t value = parse_hexint(s.c_str());
    if ( value > 0xFF ) throw nano::exception(std::string("wrong ") + type + ": " + s);
    return value & 0xFF;
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
    printf("    wfuse - write fuses from pigro.ini\n");
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
    nano::IniReader<512> config;
    nano::serial *serial = nullptr;
    AVR_Info avr;
    std::string hexfname;

    // STM32
    bool arm = false;
    uint8_t arm_memap;
    static constexpr uint32_t arm_csw = 0x22000000;

public:

    PigroApp(const char *path): config("pigro.ini"), serial (new nano::serial(path))
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
            throw nano::exception("send_packet(): send fail");
        }

        if ( nack_support )
        {
            switch ( serial->read_sync() )
            {
            case PKT_ACK: return true;
            case PKT_NACK: throw nano::exception("send_packet(): NACK");
            default: throw nano::exception("send_packet(): out of sync");
            }
        }

        return true;
    }

    void recv_packet(packet_t *pkt)
    {
        pkt->cmd = serial->read_sync();
        pkt->len = serial->read_sync();
        if ( pkt->len > PACKET_MAXLEN )
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
                    throw nano::exception("wrong protocol: len = " + std::to_string(pkt.len));
                nack_support = true;
                protoVersionMajor = pkt.data[0];
                protoVersionMinor = pkt.data[1];
                info("new protocol");
                return;
            }
            else
            {
                throw nano::exception("wrong protocol: ack = " + std::to_string(ack));
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
        cmd_jtag_reset();
        arm_check_idcode();
        arm_debug_enable();
        arm_find_memap();

        /*
        auto signature = isp_chip_info();
        if ( signature != avr.signature )
        {
            warn("isp_check_firmware(): wrong chip signature");
        }*/
        uint32_t csw = arm_memap_csw();
        printf("csw: 0x%08X\n", csw);
        csw = (csw & ~0x7) | 0;
        printf("new csw: 0x%08X\n", csw);
        arm_memap_csw(csw);


        uint8_t counter = 0;
        uint32_t memaddr = 0x08000000;
        arm_set_memaddr(memaddr);
        for(const auto &[page_addr, page] : pages)
        {
            const size_t size = page.data.size() / 4;
            for(size_t i = 0; i < size; i++)
            {
                const uint16_t offset = i * 4;
                const uint32_t addr = 0x08000000 + page_addr * 2 + offset;
                if ( counter == 0 ) printf("MEM[0x%08X]", addr);
                if ( addr != memaddr )
                {
                    arm_set_memaddr(addr);
                    memaddr = addr;
                }
                //fflush(stdout);
                uint32_t hex_value = page.data[offset] | (page.data[offset+1] << 8) | (page.data[offset+2] << 16) | (page.data[offset+3] << 24);
                uint32_t device_value = arm_read_mem32();
                memaddr += 4;
                printf("%s", (device_value == hex_value ? "." : "*" ));
                //fflush(stdout);
                //printf("counter = %d\n", counter);
                if ( counter == 0x1F ) printf("\n");
                counter = (counter + 1) & 0x1F;
            }
        }
        if ( counter != 0 )
            printf("\n");
    }

    /**
     * Записать прошивку
     */
    void isp_write_firmware(const AVR_Data &pages)
    {
        cmd_jtag_reset();
        arm_check_idcode();
        arm_debug_enable();
        arm_find_memap();

        arm_fpec_unlock();

        arm_fpec_mass_erase();

        /*
        auto signature = isp_chip_info();
        if ( signature != avr.signature )
        {
            throw nano::exception("isp_write_firmware() reject: wrong chip signature");
        }
        if ( !avr.valid() || !avr.paged )
        {
            throw nano::exception("isp_write_firmware() reject: unsupported chip");
        }*/

        //isp_chip_erase();

        arm_fpec_write_reg(0x10, 1); // FLASH_CR_PG

        printf("flash_cr: 0x%08X\n", arm_fpec_read_reg(0x10));

        uint8_t counter = 0;
        uint32_t memaddr = 0x08000000;
        arm_set_memaddr(memaddr);
        for(const auto &[page_addr, page] : pages)
        {
            const size_t size = page.data.size() / 4;
            for(size_t i = 0; i < size; i++)
            {
                const uint32_t offset = i * 4;
                const uint32_t addr = 0x08000000 + page_addr * 2 + offset;
                if ( counter == 0 ) printf("MEM[0x%08X]", addr);
                if ( addr != memaddr )
                {
                    arm_set_memaddr(addr);
                    memaddr = addr;
                }
                //fflush(stdout);
                try
                {
                    //uint32_t addr0 = addr;
                    //uint32_t addr1 = addr + 2;

                    //uint16_t word0 = page.data[offset] | (page.data[offset+1] << 8);
                    //uint16_t word1 = page.data[offset+2] | (page.data[offset+3] << 8);
                    uint32_t word = page.data[offset] | (page.data[offset+1] << 8) | (page.data[offset+2] << 16) | (page.data[offset+3] << 24);
                    //arm_fpec_program(addr0, word0);
                    //arm_fpec_program(addr1, word1);
                    arm_fpec_program_v2(word);
                    memaddr += 4;
                    printf(".");
                }
                catch (const nano::exception &e)
                {
                    printf("*\n%s\n", e.message().c_str());
                    throw;
                }
                //fflush(stdout);
                if ( counter == 0x1F )
                {
                    printf("\n");
                    //printf("\nflash_cr: 0x%08X\n", arm_fpec_read_reg(0x10));
                }
                counter = (counter + 1) & 0x1F;
            }
        }
        if ( counter != 0 )
            printf("\n");

        arm_fpec_write_reg(0x10, 0); // FLASH_CR_PG
    }


    /**
     * @brief Действие - вывести информацию об устройстве
     * @return
     */
    int action_info()
    {
        loadConfig();

        auto info = isp_chip_info();
        printf("chip signature: 0x%02X, 0x%02X, 0x%02X\n", info[0], info[1], info[2]);
        if ( info != avr.signature )
        {
            warn("isp_check_firmware(): wrong chip signature");
        }

        return 1;
    }

    uint8_t isp_read_fuse_high()
    {
        uint32_t r = cmd_isp_io(0x58080000);
        uint8_t echo = (r >> 8) & 0xFF;
        if ( echo != 0x08 ) throw nano::exception("isp_read_fuse_high() out of sync");

        return r & 0xFF;
    }

    uint8_t isp_read_fuse_low()
    {
        uint32_t r = cmd_isp_io(0x50000000);
        uint8_t echo = (r >> 8) & 0xFF;
        if ( echo != 0x00 ) throw nano::exception("isp_read_fuse_low() out of sync");

        return r & 0xFF;
    }

    uint8_t isp_read_fuse_ext()
    {
        uint32_t r = cmd_isp_io(0x50080000);
        uint8_t echo = (r >> 8) & 0xFF;
        if ( echo != 0x08 ) throw nano::exception("isp_read_fuse_ext() out of sync");

        return r & 0xFF;
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

        uint8_t fuse_lo = isp_read_fuse_low();  // cmd_isp_io(0x50000000) & 0xFF;
        uint8_t fuse_hi = isp_read_fuse_high(); // cmd_isp_io(0x58080000) & 0xFF;
        uint8_t fuse_ex = isp_read_fuse_ext();  // cmd_isp_io(0x50080000) & 0xFF;

        const char *status;

        if ( auto s = config.value("main", "fuse_low"); !s.empty() )
        {
            const uint8_t x = parse_fuse(s, "fuse_low (pigro.ini)");
            status = (x == fuse_lo) ? " ok " : "diff";
        }
        else status = " NA ";
        printf("fuse low:  0x%02X [%s]\n", fuse_lo, status);

        if ( auto s = config.value("main", "fuse_high"); !s.empty() )
        {
            const uint8_t x = parse_fuse(s, "fuse_high (pigro.ini)");
            status = (x == fuse_hi) ? " ok " : "diff";
        }
        else status = " NA ";
        printf("fuse high: 0x%02X [%s]\n", fuse_hi, status);

        if ( auto s = config.value("main", "fuse_ext"); !s.empty() )
        {
            const uint8_t x = parse_fuse(s, "fuse_ext (pigro.ini)");
            status = (x == fuse_ex) ? " ok " : "diff";
        }
        else status = " NA ";
        printf("fuse ext:  0x%02X [%s]\n", fuse_ex, status);

        return 0;
    }

    int action_write_fuse()
    {
        if ( verbose )
        {
            info("write device's fuses");
        }

        if ( auto s = config.value("main", "fuse_low"); !s.empty() )
        {
            const uint8_t x = parse_fuse(s, "fuse_low (pigro.ini)");
            isp_write_fuse_low(x);
        }

        if ( auto s = config.value("main", "fuse_high"); !s.empty() )
        {
            const uint8_t x = parse_fuse(s, "fuse_high (pigro.ini)");
            isp_write_fuse_high(x);
        }

        if ( auto s = config.value("main", "fuse_ext"); !s.empty() )
        {
            const uint8_t x = parse_fuse(s, "fuse_ext (pigro.ini)");
            isp_write_fuse_ext(x);
        }

        return 0;
    }

    void isp_write_fuse_low(uint8_t value)
    {
        unsigned int r = cmd_isp_io(0xACA00000 + (value & 0xFF));
        int ok = ((r >> 16) & 0xFF) == 0xAC;

        if ( verbose || !ok )
        {
            const char *status = ok ? "[ ok ]" : "[ fail ]";
            printf("write device's low fuse bits %s\n", status);
        }
    }

    /**
     * Действие - записать младшие биты fuse
     */
    int action_write_fuse_lo()
    {
        auto fuse_bits = parse_fuse(fuse_bits_s, "fuse");

        if ( verbose )
        {
            printf("write device's low fuse bits (0x%02X)\n", fuse_bits);
        }

        isp_write_fuse_low(fuse_bits);
        return 0;
    }

    void isp_write_fuse_high(uint8_t value)
    {
        uint32_t r = cmd_isp_io(0xACA80000 + (value & 0xFF));
        int ok = ((r >> 8) & 0xFF) == 0xA8;

        if ( verbose || !ok )
        {
            const char *status = ok ? "[ ok ]" : "[ fail ]";
            printf("write device's high fuse bits %s\n", status);
        }
    }

    /**
     * Действие - записать старшие биты fuse
     */
    int action_write_fuse_hi()
    {
        auto fuse_bits = parse_fuse(fuse_bits_s, "fuse");

        if ( verbose )
        {
            printf("write device's high fuse bits (0x%02X)\n", fuse_bits);
        }

        isp_write_fuse_high(fuse_bits);
        return 0;
    }

    void isp_write_fuse_ext(uint8_t value)
    {
        uint32_t r = cmd_isp_io(0xACA40000 + (value & 0xFF));
        int ok = ((r >> 8) & 0xFF) == 0xA4;

        if ( verbose || !ok )
        {
            const char *status = ok ? "[ ok ]" : "[ fail ]";
            printf("write device's extended fuse bits %s\n", status);
        }
    }

    /**
     * Действие - записать расширеные биты fuse
     */
    int action_write_fuse_ex()
    {
        auto fuse_bits = parse_fuse(fuse_bits_s, "fuse");

        if ( verbose )
        {
            printf("write device's extended fuse bits (0x%02X)\n", fuse_bits);
        }

        isp_write_fuse_ext(fuse_bits);
        return 0;
    }

    /**
     * Действие - стереть чип
     */
    int action_erase()
    {
        isp_chip_erase();
        return 1;
    }

    void loadConfig()
    {
        if ( const std::string output = config.value("main", "output", "quiet"); output == "verbose" )
        {
            verbose = true;
        }

        std::string device = config.value("main", "device");
        if ( device.empty() )
        {
            throw nano::exception("specify device");
        }

        if ( config.value("main", "type") == "arm" )
        {
            avr.page_word_size = 16;
            avr.page_count = 2*1024;
            avr.paged = true;
            arm = true;
        }
        else if ( auto dev = AVR::findDeviceByName(device); dev.has_value() )
        {
            avr = *dev;
        }
        else
        {
            throw nano::exception("device not found: " + device);
        }

        hexfname = config.value("main", "hex", fname);

        if ( verbose )
        {
            std::cout << "device: " << device << "\n";
            if ( avr.paged )
            {
                std::cout << "page_size: " << int(avr.page_word_size) << " words\n";
                std::cout << "page_count: " << int(avr.page_count) << "\n";
            }
            std::cout << "flash_size: " << ((avr.flash_size()+1023) / 1024) << "k\n";
            std::cout << "hex_file: " << hexfname << "\n";
        }

        /*if ( !avr.paged )
        {
            warn("unsupported chip, only Write Page supported");
        }
        else if ( !avr.valid() )
        {
            warn("invalid or unsupported chip data, check database");
        }*/

    }

    AVR_Data readHEX()
    {
        loadConfig();

        auto pages = avr_load_from_hex(avr, hexfname);
        printf("page usages: %ld / %d\n", pages.size(), avr.page_count);
        return pages;
    }

    void isp_check_fuses()
    {
        if ( auto s = config.value("main", "fuse_low"); !s.empty() )
        {
            const uint8_t fuse_lo = isp_read_fuse_low();
            const uint8_t x = parse_fuse(s, "fuse_low (pigro.ini)");
            const char *status = (x == fuse_lo) ? " ok " : "diff";
            printf("fuse low:  0x%02X [%s]\n", fuse_lo, status);
        }

        if ( auto s = config.value("main", "fuse_high"); !s.empty() )
        {
            const uint8_t fuse_hi = isp_read_fuse_high();
            const uint8_t x = parse_fuse(s, "fuse_high (pigro.ini)");
            const char *status = (x == fuse_hi) ? " ok " : "diff";
            printf("fuse high: 0x%02X [%s]\n", fuse_hi, status);
        }

        if ( auto s = config.value("main", "fuse_ext"); !s.empty() )
        {
            const uint8_t fuse_ext = isp_read_fuse_ext();
            const uint8_t x = parse_fuse(s, "fuse_ext (pigro.ini)");
            const char *status = (x == fuse_ext) ? " ok " : "diff";
            printf("fuse ext:  0x%02X [%s]\n", fuse_ext, status);
        }
    }

    /**
     * Действие - проверить прошивку в устройстве
     */
    int action_check()
    {
        AVR_Data pages = readHEX();
        //isp_check_fuses();
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

    static constexpr uint8_t IR_BYPASS = 0b1111;
    static constexpr uint8_t IR_IDCODE = 0b1110;
    static constexpr uint8_t IR_DPACC = 0b1010;
    static constexpr uint8_t IR_APACC = 0b1011;
    static constexpr uint8_t IR_ABORT = 0b1000;

    static constexpr uint8_t ACK_OKFAULT = 0b010;
    static constexpr uint8_t ACK_WAIT = 0b001;

    static constexpr uint32_t CORTEX_M3_IDCODE = 0x3BA00477;

    static constexpr uint32_t CSYSPWRUPACK = 1 << 31; // RO  System power-up acknowledge.
    static constexpr uint32_t CSYSPWRUPREQ = 1 << 30; // R/W System power-up request.
    static constexpr uint32_t CDBGPWRUPACK = 1 << 29; // Debug power-up acknowledge.
    static constexpr uint32_t CDBGPWRUPREQ = 1 << 28; // R/W Debug power-up request.

    void cmd_jtag_reset()
    {
        packet_t pkt;
        pkt.cmd = 5;
        pkt.len = 1;
        pkt.data[0] = 0;
        printf("cmd_jtag_reset()\n");
        auto status = send_packet(&pkt);
        if ( !status ) throw nano::exception("cmd_jtag_reset() fail");
    }

    uint8_t cmd_jtag_ir(uint8_t ir)
    {
        packet_t pkt;
        pkt.cmd = 6;
        pkt.len = 1;
        pkt.data[0] = ir;

        //printf("cmd_jtag_ir() send: 0x%02X\n", ir);
        send_packet(&pkt);

        recv_packet(&pkt);
        //printf("cmd_jtag_ir() recv: 0x%02X\n", pkt.data[0]);
        return pkt.data[0];
    }

    void cmd_jtag_dr(uint8_t *data, uint8_t bitcount)
    {
        const uint8_t bytecount = (bitcount+7) / 8;
        packet_t pkt;
        pkt.cmd = 7;
        pkt.len = bytecount + 1;
        if ( pkt.len > PACKET_MAXLEN ) throw nano::exception("cmd_jtag_dr() bitcount too long: " + std::to_string(bitcount));

        pkt.data[0] = bitcount;
        for(uint8_t i = 0; i < bytecount; i++)
            pkt.data[i+1] = data[i];

        /*
        printf("cmd_jtag_dr() send:");
        for(uint8_t i = 1; i < pkt.len; i++)
        {
            printf(" 0x%02X", pkt.data[i]);
        }
        printf(" (%d)\n", pkt.data[0]);
        */

        send_packet(&pkt);

        recv_packet(&pkt);

        /*
        printf("cmd_jtag_dr() recv:");
        for(uint8_t i = 1; i < pkt.len; i++)
        {
            printf(" 0x%02X", pkt.data[i]);
        }
        printf(" (%d)\n", pkt.data[0]);
        */

        for(uint8_t i = 1; i < pkt.len; i++)
        {
            data[i-1] = pkt.data[i];
        }
    }

    uint32_t arm_idcode()
    {
        cmd_jtag_ir(IR_IDCODE);
        return cmd_jtag_dr<32>(0);
    }

    void dump_packet(const char *message, const packet_t &pkt)
    {
        printf("%s [cmd=0x%02X]:", message, pkt.cmd);
        for(uint8_t i = 0; i < pkt.len; i++)
        {
            printf(" 0x%02X", pkt.data[i]);
        }
        printf("\n");
    }

    template <uint8_t bitcount>
    uint32_t arm_io(uint8_t ir, uint64_t value)
    {
        constexpr uint8_t bytecount = (bitcount + 7) / 8;
        if ( bytecount + 2 > PACKET_MAXLEN ) throw nano::exception("arm_io() packet to long: " + std::to_string(bitcount));
        packet_t pkt;
        pkt.cmd = 8;
        pkt.len = bytecount + 2;
        pkt.data[0] = ir;
        pkt.data[1] = bitcount;
        for(int i = 0; i < bytecount; i++)
        {
            pkt.data[i+2] = value & 0xFF;
            value = value >> 8;
        }

        dump_packet("arm_io() send", pkt);
        send_packet(&pkt);
        recv_packet(&pkt);
        dump_packet("arm_io() recv", pkt);
        if ( pkt.cmd != 8 ) throw nano::exception("arm_io() wrong reply: cmd=" + std::to_string(pkt.cmd));
        if ( pkt.len != bytecount + 2 ) throw nano::exception("arm_io() wrong len: " + std::to_string(pkt.len));

        uint64_t result = 0;
        uint64_t mul = 1;
        for(int i = 0; i < bytecount; i++)
        {
            result = result | (pkt.data[i+2] * mul);
            mul = mul * 256;
        }
        return result;
    }

    uint32_t arm_io32(uint8_t ir, uint32_t value)
    {
        return arm_io<32>(ir, value);
    }

    uint32_t arm_idcode_v2()
    {
        printf("\ncheck arm_idcode_v2():\n");
        uint32_t idcode = arm_io32(IR_IDCODE, 0);
        const char *status = (idcode == CORTEX_M3_IDCODE) ? "[ ok ]" : "[fail]";
        printf("idcode: 0x%08X %s\n", idcode, status);
        return idcode;
    }

    uint64_t arm_xpacc_2(uint8_t ir, uint64_t dr)
    {
        cmd_jtag_ir(ir);
        return cmd_jtag_dr<35>(dr);
    }

    const char *action(bool write)
    {
        return write ? "write" : "read";
    }

    uint32_t arm_xpacc(uint8_t ir, uint8_t reg, uint32_t value, bool write)
    {
        //printf("arm_xpacc(0x%02X, 0x%02X, 0x%08X, %s):\n", ir, reg, value, action(write));
        if ( reg > 0x0F ) throw nano::exception("arm_xpacc_ex() wrong register: " + std::to_string(reg));
        packet_t pkt;
        pkt.cmd = 9;
        pkt.len = 6;
        if ( pkt.len > PACKET_MAXLEN ) throw nano::exception("arm_xpacc() pkt.len too big: " + std::to_string(pkt.len));
        pkt.data[0] = ir;
        pkt.data[1] = ((reg / 4) << 1) | (write ? 0 : 1);
        pkt.data[2] = value & 0xFF;
        pkt.data[3] = (value >> 8) & 0xFF;
        pkt.data[4] = (value >> 16) & 0xFF;
        pkt.data[5] = (value >> 24) & 0xFF;

        //dump_packet("arm_xpacc() send", pkt);
        send_packet(&pkt);

        recv_packet(&pkt);
        //dump_packet("arm_xpacc() recv", pkt);

        if ( pkt.cmd != 9 ) throw nano::exception("arm_xpacc() wrong reply: " + std::to_string(pkt.cmd));
        if ( pkt.len == 1 ) throw nano::exception("arm_xpacc() JTAG failed: ir_ack=" + std::to_string(pkt.data[0]));
        if ( pkt.len != 6 ) throw nano::exception("arm_xpacc() wrong reply length: " + std::to_string(pkt.len));

        const uint8_t ack = pkt.data[1];
        const uint32_t data = pkt.data[2] | (pkt.data[3] << 8) | (pkt.data[4] << 16) | (pkt.data[5] << 24);

        if ( ack == ACK_OKFAULT ) return data;

        if ( ack & (1 << 3) ) throw nano::exception("arm_xpacc() command WAIT");
        if ( ack & (1 << 4) ) throw nano::exception("arm_xpacc() command wrong ack");
        if ( ack & (1 << 5) ) throw nano::exception("arm_xpacc() command failure");
        if ( ack == ACK_WAIT ) throw nano::exception("arm_xpacc() status WAIT");
        throw nano::exception("arm_xpacc() status wrong ack");
    }

    uint32_t arm_apacc(uint8_t ap, uint8_t reg, uint32_t value, bool write)
    {
        //printf("arm_apacc(ap=0x%02X, reg=0x%02X, value=0x%08X, %s):\n", ap, reg, value, action(write));
        if ( reg & 0x03 ) throw nano::exception("arm_apacc() wrong register: " + std::to_string(reg));
        packet_t pkt;
        pkt.cmd = 10;
        pkt.len = 6;
        if ( pkt.len > PACKET_MAXLEN ) throw nano::exception("arm_apacc() pkt.len too big: " + std::to_string(pkt.len));
        pkt.data[0] = ap;
        pkt.data[1] = (reg & 0xFC) | (write ? 0b00 : 0b10);
        pkt.data[2] = value & 0xFF;
        pkt.data[3] = (value >> 8) & 0xFF;
        pkt.data[4] = (value >> 16) & 0xFF;
        pkt.data[5] = (value >> 24) & 0xFF;

        //dump_packet("arm_apacc() send", pkt);
        send_packet(&pkt);

        recv_packet(&pkt);
        //dump_packet("arm_apacc() recv", pkt);

        if ( pkt.cmd != 10 ) throw nano::exception("arm_apacc() wrong reply: " + std::to_string(pkt.cmd));
        if ( pkt.len == 1 ) throw nano::exception("arm_apacc() JTAG failed: ir_ack=" + std::to_string(pkt.data[0]));
        if ( pkt.len != 6 ) throw nano::exception("arm_apacc() wrong reply length: " + std::to_string(pkt.len));

        const uint8_t ack = pkt.data[1];
        const uint32_t data = pkt.data[2] | (pkt.data[3] << 8) | (pkt.data[4] << 16) | (pkt.data[5] << 24);

        if ( ack == ACK_OKFAULT ) return data;

        if ( ack & (1 << 6) ) throw nano::exception("arm_apacc() select ap failure");
        if ( ack & (1 << 3) ) throw nano::exception("arm_apacc() command WAIT");
        if ( ack & (1 << 4) ) throw nano::exception("arm_apacc() command wrong ack");
        if ( ack & (1 << 5) ) throw nano::exception("arm_apacc() command failure");
        if ( ack == ACK_WAIT ) throw nano::exception("arm_apacc() status WAIT");
        throw nano::exception("arm_apacc() status wrong ack");
    }

    const char *ack_name(uint8_t ack)
    {
        switch (ack)
        {
        case 0b010: return "OK/FAULT";
        case 0b001: return "WAIT";
        default: return "unknown";
        }
    }

    uint32_t arm_xpacc_ex(uint8_t ir, uint8_t addr, uint32_t value, bool write)
    {
        if ( addr > 0x0F ) throw nano::exception("arm_xpacc_ex() wrong addr: " + std::to_string(addr));
        const uint64_t D = value;
        const uint64_t A = (addr >> 2) & 0x3;
        const uint64_t RW = write ? 0 : 1;
        const uint64_t cmd = (D << 3) | (A << 1) | RW;
        /*if ( write )
        {
            printf("xpacc write cmd: 0x%09lX D=%08lX A=%02lX RW=%02lX\n", cmd, D, A, RW);
        }
        else
        {
            if ( ir == IR_APACC )
                printf("xpacc AP read cmd: 0x%09lX D=%08lX A=%02lX RW=%02lX\n", cmd, D, A, RW);
        }*/
        const uint64_t result = arm_xpacc_2(ir, cmd);
        const uint8_t ack = result & 0x7;

        if ( ack == 0b010 )
        {
            return result >> 3;
        }

        if ( ack == 0b001 )
        {
            throw nano::exception("arm_xpacc_ex() ack==WAIT");
        }

        printf("ack: 0x%02X %s\n", ack, ack_name(ack));
        throw nano::exception("arm_xpacc_ex() wrong ack: " + std::to_string(ack));
    }

    static constexpr bool xpacc_read = false;
    static constexpr bool xpacc_write = true;

    uint32_t arm_read_dp(uint8_t addr)
    {
        return arm_xpacc_ex(IR_DPACC, addr, 0, xpacc_read);
    }

    uint32_t arm_read_dp_v2(uint8_t addr)
    {
        return arm_xpacc(IR_DPACC, addr, 0, xpacc_read);
    }

    void arm_write_dp(uint8_t addr, uint32_t value)
    {
        //printf("write_dp(0x%02X, 0x%08X)\n", addr, value);
        arm_xpacc_ex(IR_DPACC, addr, value, xpacc_write);
    }

    void arm_write_dp_v2(uint8_t addr, uint32_t value)
    {
        //printf("write_dp(0x%02X, 0x%08X)\n", addr, value);
        arm_xpacc(IR_DPACC, addr, value, xpacc_write);
    }

    uint32_t arm_read_ap(uint8_t addr)
    {
        //return arm_xpacc(IR_APACC, addr, 0, xpacc_read);


        arm_xpacc_ex(IR_APACC, addr, 0, xpacc_read);
        auto read2 = arm_read_dp(0x4);
        auto status = arm_read_dp(0xC);
        if ( is_error(status) )
        {
            arm_clear_sticky(status);
            throw nano::exception("arm_read_ap() fail: " + std::to_string(read2));
        }
        return read2;

    }

    void arm_write_ap(uint8_t addr, uint32_t value)
    {
        arm_xpacc_ex(IR_APACC, addr, value, xpacc_write);
        arm_read_dp(0x4); // status
        auto status = arm_read_dp(0xC);
        if ( is_error(status) )
        {
            arm_clear_sticky(status);
            throw nano::exception("arm_write_ap() fail: " + std::to_string(value));
        }
    }

    void arm_ap_select(uint8_t apsel, uint8_t addr)
    {
        const uint32_t select = uint32_t(apsel) << 24 | (addr & 0xF0);
        //printf("ap_select(): 0x%08X\n", select);
        arm_write_dp(0x8, select);
        arm_read_dp(0x4);
        auto status = arm_read_dp(0x4);
        if ( is_error(status) )
        {
            arm_clear_sticky(status);
            throw nano::exception("arm_ap_select() fail");
        }
    }

    uint32_t arm_idr(uint8_t apsel)
    {
        return arm_apacc(apsel, 0xFC, 0, xpacc_read);
        /*
        const uint8_t addr = 0xFC;
        arm_ap_select(apsel, addr);
        auto result = arm_read_ap(addr & 0x0F);
        return result;
        */
    }

    void arm_set_memap(uint8_t ap)
    {
        printf("arm_set_memap(0x%02X)\n", ap);
        packet_t pkt;
        pkt.cmd = 11;
        pkt.len = 2;
        pkt.data[0] = 1;
        pkt.data[1] = ap;
        send_packet(&pkt);
        recv_packet(&pkt);
        if ( pkt.len != 2 || pkt.data[1] != ap ) throw nano::exception("arm_set_memap() failed");
    }

    void arm_set_memaddr(uint32_t addr)
    {
        printf("arm_set_memaddr(0x%08X)\n", addr);
        packet_t pkt;
        pkt.cmd = 11;
        pkt.len = 5;
        pkt.data[0] = 2;
        pkt.data[1] = addr & 0xFF;
        pkt.data[2] = (addr >> 8) & 0xFF;
        pkt.data[3] = (addr >> 16) & 0xFF;
        pkt.data[4] = (addr >> 24) & 0xFF;
        send_packet(&pkt);
        recv_packet(&pkt);
        //dump_packet("arm_set_memaddr() recv:", pkt);
        if ( pkt.len != 5 ) throw nano::exception("arm_set_memaddr() wrong length: " + std::to_string(pkt.len));
        uint32_t output = pkt.data[1] | (pkt.data[2] << 8) | (pkt.data[3] << 16) | (pkt.data[4] << 24);
        if ( output != addr ) throw nano::exception("arm_set_memaddr() failed");
    }

    uint32_t arm_read_mem32()
    {
        //printf("arm_read_mem32()\n");
        packet_t pkt;
        pkt.cmd = 12;
        pkt.len = 4;
        send_packet(&pkt);
        recv_packet(&pkt);
        //dump_packet("arm_read_mem32() recv", pkt);
        if ( pkt.len != 4 ) throw nano::exception("arm_read_mem32() wrong length: " + std::to_string(pkt.len));
        return pkt.data[0] | (pkt.data[1] << 8) | (pkt.data[2] << 16) | (pkt.data[3] << 24);
    }

    uint16_t arm_read_mem16()
    {
        packet_t pkt;
        pkt.cmd = 12;
        pkt.len = 2;
        send_packet(&pkt);
        recv_packet(&pkt);
        if ( pkt.len != 2 ) throw nano::exception("arm_read_mem16() wrong length: " + std::to_string(pkt.len));
        return pkt.data[0] | (pkt.data[1] << 8);
    }

    void arm_write_mem32(uint32_t value)
    {
        packet_t pkt;
        pkt.cmd = 13;
        pkt.len = 4;
        pkt.data[0] = value & 0xFF;
        pkt.data[1] = (value >> 8) & 0xFF;
        pkt.data[2] = (value >> 16) & 0xFF;
        pkt.data[3] = (value >> 24) & 0xFF;
        send_packet(&pkt);
        recv_packet(&pkt);
        if ( pkt.len != 4 ) throw nano::exception("arm_write_mem32() wrong length: " + std::to_string(pkt.len));
        uint32_t output = pkt.data[0] | (pkt.data[1] << 8) | (pkt.data[2] << 16) | (pkt.data[3] << 24);
        if ( output != value ) throw nano::exception("arm_write_mem32() failed");
    }

    void arm_write_mem16(uint16_t value)
    {
        packet_t pkt;
        pkt.cmd = 13;
        pkt.len = 2;
        pkt.data[0] = value & 0xFF;
        pkt.data[1] = (value >> 8) & 0xFF;
        send_packet(&pkt);
        recv_packet(&pkt);
        if ( pkt.len != 2 ) throw nano::exception("arm_write_mem16() wrong length: " + std::to_string(pkt.len));
        uint16_t output = pkt.data[0] | (pkt.data[1] << 8);
        if ( output != value ) throw nano::exception("arm_write_mem16() failed");
    }

    void arm_check_idcode()
    {
        printf("\ncheck idcode:\n");
        const uint32_t idcode = arm_idcode();
        const char *status = (idcode == CORTEX_M3_IDCODE) ? "[ ok ]" : "[fail]";
        printf("idcode: 0x%08X %s\n", idcode, status);
    }

    template <int bitcount>
    void arm_check_bypass(uint64_t value)
    {
        printf("\ncheck bypass%d(0x%08lX):\n", bitcount, value);
        const auto result = arm_bypass<bitcount>(value);
        const char *status = (result == ((value << 2)& bitmask<bitcount>())) ? "[ ok ]" : "[fail]";
        printf("result: 0x%08lX %s\n", result, status);
    }

    template <int bitcount>
    uint64_t cmd_jtag_dr(uint64_t dr)
    {
        constexpr int bytecount = (bitcount+7) / 8;
        uint8_t data[bytecount];
        for(int i = 0; i < bytecount; i++)
        {
            data[i] = dr & 0xFF;
            dr = dr >> 8;
        }

        cmd_jtag_dr(data, bitcount);

        uint64_t result = 0;
        uint64_t mul = 1;
        for(int i = 0; i < bytecount; i++)
        {
            result = result | (data[i] * mul);
            mul = mul * 256;
        }
        return result;
    }

    uint32_t arm_bypass(uint32_t value)
    {
        cmd_jtag_ir(IR_BYPASS);
        return cmd_jtag_dr<32>(value);
    }

    template <int bitcount>
    static constexpr uint64_t bitmask()
    {
        return (uint64_t(1) << bitcount) - 1;
    }

    template <int bitcount>
    uint64_t arm_bypass(uint64_t value)
    {
        if ( value > bitmask<bitcount>() ) throw nano::exception("bypass<"+std::to_string(bitcount)+"> value too big: " + std::to_string(value));
        cmd_jtag_ir(IR_BYPASS);
        return cmd_jtag_dr<bitcount>(value);
    }

    uint64_t arm_bypass35(uint64_t value)
    {
        return arm_bypass<35>(value);
    }

    uint32_t arm_status()
    {
        return arm_read_dp_v2(0x4);
    }

    bool is_error(uint32_t status)
    {
        const uint32_t mask = (1 << 5) | (1 << 4) | (1 << 1);
        return (status & mask) != 0;
    }

    void arm_clear_sticky(uint32_t status)
    {
        arm_write_dp(0x4, status);
    }

    void arm_debug_enable()
    {
        uint32_t status = CSYSPWRUPREQ | CDBGPWRUPREQ;
        arm_write_dp_v2(0x4, status);

        status = arm_status();
        printf("status: 0x%08X\n", status);

        if ( (status & CDBGPWRUPACK) == 0 )
        {
            throw nano::exception("no debug power");
        }
        if ( (status & CSYSPWRUPACK) == 0 )
        {
            throw nano::exception("no system power");
        }
    }

    void arm_find_memap()
    {
        printf("\nfind MEM-AP\n");
        for(int i = 0; i < 256; i++)
        {
            try
            {
                const uint32_t idr = arm_idr(i);
                const uint32_t test = (idr >> 16) & 0xFFF;
                printf("idr(%d) = 0x%08X test=0x%03X\n", i, idr, test);
                if ( idr == 0 ) break;


                if ( test == 0x477 )
                {
                    arm_memap = i;
                    printf("MEM-AP: %d\n", arm_memap);
                    return;
                }

            }
            catch(const nano::exception &e)
            {
                printf("idr(%d) error: %s\n", i, e.message().c_str());
                break;
            }

        }

        throw nano::exception("MEM-AP not found");

    }

    uint32_t arm_read_memap(uint8_t addr)
    {
        return arm_apacc(arm_memap, addr, 0, xpacc_read);
        /*
        arm_ap_select(arm_memap, addr);
        return arm_read_ap(addr & 0x0F);
        */
    }

    void arm_write_memap(uint8_t addr, uint32_t value)
    {
        arm_apacc(arm_memap, addr, value, xpacc_write);
        /*
        arm_ap_select(arm_memap, addr);
        arm_write_ap(addr & 0x0F, value);
        */
    }

    uint32_t arm_memap_csw()
    {
        return arm_read_memap(0x00);
    }

    void arm_memap_csw(uint32_t value)
    {
        arm_write_memap(0x00, value);
    }

    uint32_t arm_memap_tar()
    {
        return arm_read_memap(0x04);
    }

    void arm_memap_tar(uint32_t value)
    {
        arm_write_memap(0x04, value);
    }

    uint32_t arm_memap_drw()
    {
        return arm_read_memap(0x0C);
    }

    void arm_memap_drw(uint32_t value)
    {
        arm_write_memap(0x0C, value);
    }

    uint32_t arm_mem_read(uint32_t addr)
    {
        arm_memap_tar(addr);
        return arm_memap_drw();
    }

    void arm_mem_write(uint32_t addr, uint32_t value)
    {
        arm_memap_tar(addr);
        arm_memap_drw(value);
    }

    uint8_t arm_mem_read8(uint32_t addr)
    {
        arm_memap_csw(arm_csw | 2);
        uint32_t data = arm_mem_read(addr & ~0x3);
        switch( addr & 0x3 )
        {
        case 0: return data & 0xFF;
        case 1: return (data >> 8) & 0xFF;
        case 2: return (data >> 16) & 0xFF;
        case 3: return (data >> 24) & 0xFF;
        default:
            throw nano::exception("arm_mem_read8() unexpected");
        }
    }

    uint32_t arm_flash_size()
    {
        arm_memap_csw(arm_csw | 2);
        return (arm_mem_read(0x1FFFF7E0) & 0xFFFF) * 1024;
    }

    void arm_fpec_unlock()
    {
        arm_memap_csw(arm_csw | 2);

        const uint32_t KEY1 = 0x45670123;
        const uint32_t KEY2 = 0xCDEF89AB;
        printf("\nunlock FPEC\n");
        auto flash_cr = arm_mem_read(0x40022000 + 0x10);
        if ( (flash_cr & (1 << 7)) == 0 )
        {
            printf("already unlocked\n");
            return;
        }

        arm_mem_write(0x40022000 + 4, KEY1);
        arm_mem_write(0x40022000 + 4, KEY2);

        flash_cr = arm_mem_read(0x40022000 + 0x10);
        if ( flash_cr & (1 << 7) )
        {
            throw nano::exception("arm_fpec_unlock() failed");
        }

        printf("FPEC unlocked\n");
    }

    uint32_t arm_fpec_read_reg(uint8_t reg)
    {
        arm_set_memaddr(0x40022000 + reg);
        return arm_read_mem32();
    }

    void arm_fpec_write_reg(uint8_t reg, const uint32_t &value)
    {
        arm_set_memaddr(0x40022000 + reg);
        arm_write_mem32(value);
    }

    void arm_fpec_mass_erase()
    {
        printf("\narm_fpec_mass_erase()\n");
        arm_fpec_reset_sr();
        arm_mem_write(0x40022000 + 0x10, (1 << 2)); // FLASH_CR_MER
        arm_mem_write(0x40022000 + 0x10, (1 << 2) | (1 << 6)); // FLASH_CR_STRT
        arm_fpec_check_sr();
    }

    void arm_fpec_reset_sr()
    {
        arm_memap_csw(arm_csw | 2);
        arm_mem_write(0x40022000 + 0x0C, (1 << 2) | (1 << 4) | (1 << 5));
    }

    void arm_fpec_check_sr()
    {
        arm_memap_csw(arm_csw | 2);
        auto flash_sr = arm_mem_read(0x40022000 + 0x0C);

        if ( flash_sr & 1 )
        {
            printf("flash_sr: 0x%08X\n", flash_sr);
            throw nano::exception("FPEC is busy");
        }

        if ( flash_sr & (1 << 2) )
        {
            printf("flash_sr: 0x%08X\n", flash_sr);
            throw nano::exception("PGERR: Programming error (cell already programmed)");
        }

        if ( flash_sr & (1 << 4) )
        {
            printf("flash_sr: 0x%08X\n", flash_sr);
            throw nano::exception("WRPRTERR: Write protection error");
        }

        if ( flash_sr & (1 << 5) )
        {
            //printf("EOP: Flash operation (programming / erase) is completed\n");
            return;
        }

        printf("flash_sr: 0x%08X\n", flash_sr);
        printf("FPEC status unknown\n");
    }

    void arm_fpec_program(uint32_t addr, uint16_t value)
    {
        //printf("\narm_fpec_program(0x%08X, 0x%04X)\n", addr, value);
        if ( addr & 1 ) throw nano::exception("arm_fpec_program() unaligned address");

        //printf("flash_cr: 0x%08X\n", arm_mem_read(0x40022000 + 0x10));
        arm_mem_write(0x40022000 + 0x10, 1); // FLASH_CR_PG
        //printf("flash_cr: 0x%08X\n", arm_mem_read(0x40022000 + 0x10));

        arm_fpec_reset_sr();

        arm_memap_csw(arm_csw | 1);
        uint32_t data = (addr & 2) ? ((uint32_t(value) << 16)) : (value);
        //printf("mem_write(0x%08X, 0x%08X)\n", addr, data);
        arm_mem_write(addr, data);

        //arm_memap_csw(arm_csw | 2);
        //printf("flash_cr: 0x%08X\n", arm_mem_read(0x40022000 + 0x10));
        arm_fpec_check_sr();
    }

    void arm_fpec_program_v2(uint32_t value)
    {
        packet_t pkt;
        pkt.cmd = 14;
        pkt.len = 4;
        pkt.data[0] = value & 0xFF;
        pkt.data[1] = (value >> 8) & 0xFF;
        pkt.data[2] = (value >> 16) & 0xFF;
        pkt.data[3] = (value >> 24) & 0xFF;
        //dump_packet("\narm_fpec_program_v2() send", pkt);
        send_packet(&pkt);
        recv_packet(&pkt);
        //dump_packet("arm_fpec_program_v2() recv", pkt);
        if ( pkt.cmd != 14 ) throw nano::exception("arm_fpec_program_v2() wrong cmd=" + std::to_string(pkt.cmd));
        if ( pkt.len != 4 ) throw nano::exception("arm_fpec_program_v2() wrong length: " + std::to_string(pkt.len));
        const uint32_t data = pkt.data[0] | (pkt.data[1] << 8) | (pkt.data[2] << 16) | (pkt.data[3] << 24);
        if ( data != value ) throw nano::exception("arm_fpec_program_v2() failed");
    }

    void arm_dump_mem(uint32_t addr)
    {
        arm_memap_csw(arm_csw | 2);
        const uint32_t value = arm_mem_read(addr);
        printf("MEM[0x%08X]: 0x%08X\n", addr, value);
    }

    int action_test_arm()
    {
        printf("test STM32/JTAG\n");

        //arm_idcode_v2();

        arm_idcode_v2();
        cmd_jtag_reset();
        arm_check_idcode();
        cmd_jtag_reset();
        arm_debug_enable();
        arm_find_memap();

        printf("flash size: %dkB\n", (arm_flash_size()+1023)/1024);

        //arm_csw = 0x22000000; // arm_memap_csw();
        //printf("orig csw: 0x%08X\n", arm_csw);
        //arm_csw = arm_csw & ~0x7;
        //arm_memap_csw(arm_csw | 2);

        //arm_fpec_unlock();

        /*
        arm_fpec_mass_erase();
        arm_fpec_program(0x08000000, 0x0102);
        arm_fpec_program(0x08000002, 0x0304);
        arm_fpec_program(0x08000004, 0x0506);
        arm_fpec_program(0x08000006, 0x0708);
        arm_fpec_program(0x08000008, 0x090A);
        arm_fpec_program(0x0800000A, 0x0B0C);
        */

        for(int i = 0; i < 4; i++)
        {
            uint32_t addr = 0x08000000 + i*4;
            arm_memap_csw(arm_csw | 2);
            uint32_t value = arm_mem_read(addr);
            printf("MEM[0x%08X]: 0x%08X\n", addr, value);
        }

        printf("csw: 0x%08X\n", arm_memap_csw());

        arm_memap_csw(arm_csw | 2);
        //printf("write mem:\n");
        //arm_mem_write(0x40010800, 0x44444442);

        uint32_t value = arm_mem_read(0xE0042000);
        printf("MEM[0xE0042000]: 0x%08X\n", value);
        arm_dump_mem(0x40010800);

        //arm_clear_sticky(arm_status());


        //arm_check_bypass<32>(0x01020304);
        //arm_check_bypass<35>(0x101010101);
        //arm_check_bypass<38>(0x7FFFFFFFF);

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
        case AT_ACT_WRITE_FUSE: return action_write_fuse();
        case AT_ACT_WRITE_FUSE_LO: return action_write_fuse_lo();
        case AT_ACT_WRITE_FUSE_HI: return action_write_fuse_hi();
        case AT_ACT_WRITE_FUSE_EX: return action_write_fuse_ex();
        case AT_ACT_ARM: return action_test_arm();
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
    else if ( strcmp(argv[1], "wfuse") == 0 ) action = AT_ACT_WRITE_FUSE;
	else if ( strcmp(argv[1], "wfuse_lo") == 0 ) action = AT_ACT_WRITE_FUSE_LO;
	else if ( strcmp(argv[1], "wfuse_hi") == 0 ) action = AT_ACT_WRITE_FUSE_HI;
    else if ( strcmp(argv[1], "wfuse_ex") == 0 ) action = AT_ACT_WRITE_FUSE_EX;
    else if ( strcmp(argv[1], "arm") == 0 ) action = AT_ACT_ARM;
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

    if ( action != AT_ACT_ARM )
    {
        //app.isp_program_enable();
    }

    app.execute(action);

    if ( action != AT_ACT_ARM )
    {
        //app.cmd_isp_reset(1);
    }

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
