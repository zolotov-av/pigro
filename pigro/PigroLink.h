#ifndef PIGROLINK_H
#define PIGROLINK_H

#include <QtSerialPort>
#include <QString>
#include <cstdint>
#include <string>
#include <nano/config.h>

constexpr auto PACKET_MAXLEN = 12;

struct packet_t
{
    unsigned char cmd;
    unsigned char len;
    unsigned char data[PACKET_MAXLEN];
};

class PigroApp;

class PigroLink: public QObject
{
    friend class PigroDriver;

private:

    PigroApp *m_app;

    QSerialPort *serial { new QSerialPort(this) };

    bool nack_support { false };

    uint8_t m_protoVersionMajor { 0 };
    uint8_t m_protoVersionMinor { 0 };

    uint8_t readBlocked();

    void checkProtoVersion();

protected:

    /**
     * Отправить пакет данных
     */
    bool send_packet(const packet_t *pkt);

    /**
     * Прочитать пакет данных
     */
    void recv_packet(packet_t *pkt);

public:

    void info(const char *message);

    void warn(const char *message);
    void error(const char *message);

    PigroLink(PigroApp *parent);
    PigroLink(const PigroLink &) = delete;
    PigroLink(PigroLink &&) = delete;

    ~PigroLink();

    PigroLink& operator = (const PigroLink &) = delete;
    PigroLink& operator = (PigroLink &&) = delete;

    bool open(const QString &device);
    void close();

    QString errorString()
    {
        return serial->errorString();
    }

    bool verbose() const;

    std::string get_option(const std::string &name, const std::string &default_value = {});

    const nano::options& chip_info() const;

    void beginProgress(int min, int max);
    void reportProgress(int value);
    void reportMessage(const QString &message);
    void endProcess();

    uint8_t protoVersionMajor() const { return m_protoVersionMajor; }
    uint8_t protoVersionMinor() const { return m_protoVersionMinor; }

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

};

#endif // PIGROLINK_H
