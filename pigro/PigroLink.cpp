#include "PigroLink.h"
#include "Pigro.h"
#include <nano/exception.h>
#include "trace.h"

constexpr uint8_t PKT_ACK = 1;
constexpr uint8_t PKT_NACK = 2;


uint8_t PigroLink::readBlocked()
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

void PigroLink::checkProtoVersion()
{
    serial->waitForReadyRead(200);
    serial->readAll();

    nack_support = false;

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
}

void PigroLink::serialErrorOccurred(QSerialPort::SerialPortError error)
{
    if ( error == QSerialPort::NoError )
    {
        trace::log(QStringLiteral("PigroLink::serialErrorOccurred(NoError): %1").arg(serial->errorString()));
        return;
    }

    const std::string error_message = serial->errorString().toStdString();
    char buf[128];
    const int len = snprintf(buf, sizeof(buf), "serial port error: %s", error_message.c_str());
    emit errorOccurred(QString::fromUtf8(buf, len));
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
        switch ( readBlocked() )
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
    pkt->cmd = readBlocked();
    pkt->len = readBlocked();
    if ( pkt->len > PACKET_MAXLEN )
    {
        throw nano::exception("packet to big: " + std::to_string(pkt->len) + "/" + std::to_string((PACKET_MAXLEN)));
    }
    for(int i = 0; i < pkt->len; i++)
    {
        pkt->data[i] = readBlocked();
    }
}

PigroLink::PigroLink(QObject *parent): QObject(parent)
{
    connect(serial, &QSerialPort::errorOccurred, this, &PigroLink::serialErrorOccurred, Qt::DirectConnection);

    trace::log("PigroLink created");
}

PigroLink::~PigroLink()
{
    trace::log("PigroLink destroyed");
}

QString PigroLink::protoVersion() const
{
    if ( nack_support )
    {
        return QStringLiteral("%1.%2 (NACK support)").arg(protoVersionMajor()).arg(protoVersionMinor());
    }
    else
    {
        return QStringLiteral("%1.%2").arg(protoVersionMajor()).arg(protoVersionMinor());
    }
}

bool PigroLink::open(const QString &tty)
{
    serial->setPortName(tty);
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    if ( serial->open(QIODevice::ReadWrite) )
    {
        trace::log(QStringLiteral("PigroLink %1 opened").arg(serial->portName()));
        try
        {
            checkProtoVersion();
            emit sessionStarted(protoVersionMajor(), protoVersionMinor());
            return true;
        }
        catch (const std::exception &e)
        {
            serial->close();
            emit errorOccurred(QStringLiteral("error: %1").arg(e.what()));
            return false;
        }
        catch (...)
        {
            serial->close();
            emit errorOccurred(QStringLiteral("unknown exception"));
            return false;
        }
    }

    emit errorOccurred(QStringLiteral("Unable open serial port: ").append(serial->errorString()));
    return false;
}

void PigroLink::close()
{
    if ( serial->isOpen() )
    {
        emit sessionStopped();
        serial->close();
        trace::log(QStringLiteral("PigroLink %1 closed").arg(serial->portName()));
    }
}
