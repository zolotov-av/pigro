#include "PigroLink.h"
#include "PigroApp.h"
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
            emit m_app->sessionStarted(protoVersionMajor(), protoVersionMinor());
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
    emit m_app->sessionStarted(protoVersionMajor(), protoVersionMinor());
    warn("old protocol, update pigro's firmware");
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

void PigroLink::info(const char *message)
{
    if ( verbose() )
    {
        emit m_app->reportMessage(tr("info: ") + tr(message));
    }
}

void PigroLink::warn(const char *message)
{
    emit m_app->reportMessage(tr("warn: ") + tr(message));
}

void PigroLink::error(const char *message)
{
    emit m_app->reportMessage(tr("error: ") + tr(message));
}

PigroLink::PigroLink(PigroApp *parent): QObject(parent), m_app(parent)
{
    trace::log("PigroLink create");
}

PigroLink::PigroLink(PigroApp *app, QObject *parent): QObject(parent), m_app(app)
{
    trace::log("PigroLink(app, parent) create");
}

PigroLink::~PigroLink()
{
    trace::log("PigroLink destroy");
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

bool PigroLink::open()
{
    serial->setPortName(m_tty);
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    if ( serial->open(QIODevice::ReadWrite) )
    {
        trace::log(QStringLiteral("PigroLink %1 opened").arg(serial->portName()));
        try
        {
            checkProtoVersion();
            return true;
        }
        catch (const std::exception &e)
        {
            serial->close();
            emit m_app->reportMessage(QStringLiteral("error: %s").arg(e.what()));
            return false;
        }
        catch (...)
        {
            serial->close();
            emit m_app->reportMessage(QStringLiteral("unknown exception"));
            return false;
        }
    }

    emit m_app->reportMessage(QStringLiteral("Unable open serial port: ").append(serial->errorString()));
    return false;
}

void PigroLink::close()
{
    if ( serial->isOpen() )
    {
        serial->close();
        trace::log(QStringLiteral("PigroLink %1 closed").arg(serial->portName()));
    }
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

