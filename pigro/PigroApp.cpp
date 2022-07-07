#include "PigroApp.h"
#include "trace.h"

void PigroApp::threadStarted()
{
    trace::setThreadName("pigro");
    emit reportMessage("PigroApp::threadStarted()");

    m_private = new Pigro(nullptr);

    connect(m_private, &Pigro::sessionStarted, this, &PigroApp::sessionStarted, Qt::DirectConnection);
    connect(m_private, &Pigro::sessionStopped, this, &PigroApp::sessionStopped, Qt::DirectConnection);
    connect(m_private, &Pigro::beginProgress, this, &PigroApp::beginProgress, Qt::DirectConnection);
    connect(m_private, &Pigro::reportProgress, this, &PigroApp::reportProgress, Qt::DirectConnection);
    connect(m_private, &Pigro::reportMessage, this, &PigroApp::reportMessage, Qt::DirectConnection);
    connect(m_private, &Pigro::endProgress, this, &PigroApp::endProgress, Qt::DirectConnection);
    connect(m_private, &Pigro::chipInfo, this, &PigroApp::chipInfo, Qt::DirectConnection);
    connect(m_private, &Pigro::dataReady, this, &PigroApp::dataReady, Qt::DirectConnection);

    emit started();
}

void PigroApp::threadFinished()
{
    emit reportMessage("PigroApp::threadFinished()");
    delete m_private;
    m_private = nullptr;
    emit stopped();
}

FirmwareData PigroApp::getFirmwareData()
{
    const std::lock_guard lock(m_mutex);
    return m_private->fetchFirmwareData();
}

PigroApp::PigroApp(QObject *parent): QObject(parent)
{
    m_thread->setObjectName("PigroThread");

    connect(m_thread, &QThread::started, this, &PigroApp::threadStarted, Qt::DirectConnection);
    connect(m_thread, &QThread::finished, this, &PigroApp::threadFinished, Qt::DirectConnection);
}

PigroApp::~PigroApp()
{
}
