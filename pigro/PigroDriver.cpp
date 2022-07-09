#include "PigroDriver.h"
#include "Pigro.h"

void PigroDriver::info(const char *msg) const
{
    if ( verbose() )
    {
        emit m_owner->reportMessage(QStringLiteral("info: %1").arg(msg));
    }
}

void PigroDriver::warn(const char *msg)
{
    emit m_owner->reportMessage(QStringLiteral("warn: %1").arg(msg));
}

void PigroDriver::error(const char *msg) const
{
    emit m_owner->reportMessage(QStringLiteral("error: %1").arg(msg));
}

void PigroDriver::beginProgress(int min, int max)
{
    emit m_owner->beginProgress(min, max);
}

void PigroDriver::reportProgress(int value)
{
    emit m_owner->reportProgress(value);
}

void PigroDriver::reportMessage(const QString &message)
{
    emit m_owner->reportMessage(message);
}

void PigroDriver::endProgress()
{
    emit m_owner->endProgress();
}

PigroDriver::PigroDriver(PigroLink *link, Pigro *owner): m_owner(owner), m_link(link)
{

}

PigroDriver::~PigroDriver()
{

}

void PigroDriver::cancel()
{
    m_cancel = true;
    reportMessage("PigroDriver::cancel()...");
}

uint8_t PigroDriver::page_fill() const
{
    return 0xFF;
}
