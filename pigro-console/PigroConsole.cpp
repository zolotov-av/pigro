#include "PigroConsole.h"
#include <pigro/trace.h>

void PigroConsole::pigroStarted()
{
    trace::log("PigroConsole::pigroStarted()");
}

void PigroConsole::pigroStopped()
{
    trace::log("PigroConsole::pigroStopped()");
}

void PigroConsole::sessionStarted(int major, int minor)
{
    printf("pigro protocol: %d.%d\n", major, minor);
}

void PigroConsole::reportMessage(const QString &message)
{
    const std::string msg = message.toStdString();
    printf("%s\n", msg.c_str());
}

PigroConsole::PigroConsole(QObject *parent): QObject(parent)
{
    connect(link, &PigroApp::started, this, &PigroConsole::pigroStarted);
    connect(link, &PigroApp::stopped, this, &PigroConsole::pigroStopped);
    connect(link, &PigroApp::sessionStarted, this, &PigroConsole::sessionStarted);
    connect(link, &PigroApp::reportMessage, this, &PigroConsole::reportMessage);
}

PigroConsole::~PigroConsole()
{
}

int PigroConsole::exec(PigroAction action)
{
    m_action = action;

    link->start();
    link->setVerbose(m_verbose);
    link->open("/dev/ttyUSB0", "pigro.ini");
    link->execute(m_action);
    link->stop();
    link->wait();
    return 0;
}
