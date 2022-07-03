#include "PigroApp.h"
#include "trace.h"

std::atomic<int> PigroApp::m_init_counter { 0 };

void PigroApp::threadStarted()
{
    trace::setThreadName("pigro");
    trace::log("PigroApp::threadStarted()");
    m_private = new PigroPrivate(this);
    emit started();
}

void PigroApp::threadFinished()
{
    trace::log("PigroApp::threadFinished()");
    delete m_private;
    m_private = nullptr;
    emit stopped();
}

PigroApp::PigroApp(QObject *parent): QObject(parent)
{
    if ( ++m_init_counter == 1 )
    {
        trace::log("Q_INIT_RESOURCE(PigroResources)");
        Q_INIT_RESOURCE(PigroResources);
    }

    connect(m_thread, &QThread::started, this, &PigroApp::threadStarted, Qt::DirectConnection);
    connect(m_thread, &QThread::finished, this, &PigroApp::threadFinished, Qt::DirectConnection);
}

PigroApp::~PigroApp()
{
    if ( driver ) delete driver;
    if ( --m_init_counter == 0 )
    {
        trace::log("Q_CLEANUP_RESOURCE(PigroResources)");
        Q_CLEANUP_RESOURCE(PigroResources);
    }
}

void PigroApp::open(const char *ttyPath, const char *configPath)
{
    setTTY(ttyPath);
    setProjectPath(configPath);
}

void PigroApp::loadConfig()
{
    if ( !open() )
    {
        throw nano::exception(link->errorString().toStdString());
    }

    FirmwareInfo firmwareInfo{ link->projectPath() };
    link->setFirmwareInfo(firmwareInfo);

    if ( driver ) delete driver;
    driver = lookupDriver(firmwareInfo);
}

void PigroApp::loadConfig(const QString &path)
{
    link->setProjectPath(path);
    loadConfig();
}

