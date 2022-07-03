#include "PigroApp.h"
#include "trace.h"
#include <QDebug>

std::atomic<int> PigroApp::m_init_counter { 0 };

void PigroApp::threadStarted()
{
    trace::setThreadName("pigro");
    trace::log("PigroApp::threadStarted()");
    m_private = new PigroPrivate();
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
        qDebug() << "Q_INIT_RESOURCE(PigroResources)";
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
        qDebug() << "Q_CLEANUP_RESOURCE(PigroResources)";
        Q_CLEANUP_RESOURCE(PigroResources);
    }
}

void PigroApp::open(const char *ttyPath, const char *configPath)
{
    projectPath = configPath;

    if ( !open(ttyPath) )
    {
        throw nano::exception(link->errorString().toStdString());
    }
}

void PigroApp::loadConfig()
{
    firmwareInfo.loadFromFile(projectPath);

    if ( driver ) delete driver;
    driver = lookupDriver(firmwareInfo);
}

void PigroApp::loadConfig(const QString &path)
{
    projectPath = path;
    loadConfig();
}

