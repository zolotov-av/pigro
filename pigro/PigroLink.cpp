#include "PigroLink.h"
#include "PigroApp.h"
#include "trace.h"

constexpr uint8_t PKT_ACK = 1;
constexpr uint8_t PKT_NACK = 2;


uint8_t PigroLink::read_sync()
{
    char data;
    if ( serial->read(&data, 1) == 1 )
    {
        return data;
    }

    if ( serial->waitForReadyRead(200) )
    {
        char data;
        if ( serial->read(&data, 1) == 1 )
        {
            return data;
        }

        throw nano::exception("read fail: " + serial->errorString().toStdString());
    }
    else
    {
        // timeout
        throw nano::exception("read timeout");
    }
}

bool PigroLink::send_packet(const packet_t *pkt)
{
    ssize_t r = serial->write(reinterpret_cast<const char *>(pkt), pkt->len + 2);
    serial->waitForBytesWritten(200);
    if ( r != pkt->len + 2 )
    {
        // TODO обработка ошибок
        throw nano::exception("send_packet(): send fail");
    }

    if ( nack_support )
    {
        switch ( read_sync() )
        {
        case PKT_ACK: return true;
        case PKT_NACK: throw nano::exception("send_packet(): NACK");
        default: throw nano::exception("send_packet(): out of sync");
        }
    }

    return true;
}

void PigroLink::recv_packet(packet_t *pkt)
{
    pkt->cmd = read_sync();
    pkt->len = read_sync();
    if ( pkt->len > PACKET_MAXLEN )
    {
        throw nano::exception("packet to big: " + std::to_string(pkt->len) + "/" + std::to_string((PACKET_MAXLEN)));
    }
    for(int i = 0; i < pkt->len; i++)
    {
        pkt->data[i] = read_sync();
    }
}

PigroLink::PigroLink(PigroApp *parent): QObject(parent), m_app(parent)
{
    trace::log("PigroLink create");
}

PigroLink::~PigroLink()
{
    trace::log("PigroLink destroy");
}

bool PigroLink::open(const QString &device)
{
    serial->setPortName(device);
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    return serial->open(QIODevice::ReadWrite);
}

void PigroLink::close()
{
    serial->close();
}

bool PigroLink::verbose() const
{
    return m_app->verbose();
}

std::string PigroLink::get_option(const std::string &name, const std::string &default_value)
{
    return m_app->get_option(name, default_value);
}

const nano::options &PigroLink::chip_info() const
{
    return m_app->chip_info();
}

void PigroLink::beginProgress(int min, int max)
{
    emit m_app->beginProgress(min, max);
}

void PigroLink::reportProgress(int value)
{
    emit m_app->reportProgress(value);
}

void PigroLink::reportMessage(const QString &message)
{
    emit m_app->reportMessage(message);
}

void PigroLink::endProcess()
{
    emit m_app->endProgress();
}

void PigroLink::checkProtoVersion()
{
    serial->waitForReadyRead(200);
    serial->readAll();

    packet_t pkt;
    pkt.cmd = 1;
    pkt.len = 2;
    pkt.data[0] = 0;
    pkt.data[1] = 0;
    send_packet(&pkt);

    if ( serial->waitForReadyRead(200) )
    {
        char ack;
        serial->read(&ack, sizeof(ack));
        if ( ack == PKT_ACK )
        {
            recv_packet(&pkt);
            if ( pkt.len != 2 )
                throw nano::exception("wrong protocol: len = " + std::to_string(pkt.len));
            nack_support = true;
            m_protoVersionMajor = pkt.data[0];
            m_protoVersionMinor = pkt.data[1];
            info("new protocol");
            return;
        }
        else
        {
            throw nano::exception("wrong protocol: ack = " + std::to_string(ack));
        }
    }

    nack_support = false;
    m_protoVersionMajor = 0;
    m_protoVersionMinor = 1;
    warn("old protocol? update firmware...");
}
