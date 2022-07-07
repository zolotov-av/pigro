#ifndef PIGROLINK_H
#define PIGROLINK_H

#include <QSerialPort>

constexpr auto PACKET_MAXLEN = 12;

struct packet_t
{
    unsigned char cmd;
    unsigned char len;
    unsigned char data[PACKET_MAXLEN];
};

class PigroLink: public QObject
{
    Q_OBJECT

private:

    QSerialPort *serial { new QSerialPort(this) };

    bool nack_support { false };

    uint8_t m_protoVersionMajor { 0 };
    uint8_t m_protoVersionMinor { 0 };

    uint8_t readBlocked();

    void checkProtoVersion();

private slots:

    void serialErrorOccurred(QSerialPort::SerialPortError error);

public:

    QString protoVersion() const;
    uint8_t protoVersionMajor() const { return m_protoVersionMajor; }
    uint8_t protoVersionMinor() const { return m_protoVersionMinor; }

    QString errorString()
    {
        return serial->errorString();
    }

    explicit PigroLink(QObject *parent = nullptr);
    PigroLink(const PigroLink &) = delete;
    PigroLink(PigroLink &&) = delete;

    ~PigroLink();

    PigroLink& operator = (const PigroLink &) = delete;
    PigroLink& operator = (PigroLink &&) = delete;

    bool open(const QString &tty);

    /**
     * Отправить пакет данных
     */
    bool send_packet(const packet_t *pkt);

    /**
     * Прочитать пакет данных
     */
    void recv_packet(packet_t *pkt);

    void close();

signals:

    void sessionStarted(int major, int minor);
    void sessionStopped();
    void errorOccurred(const QString &message);

};

#endif // PIGROLINK_H
