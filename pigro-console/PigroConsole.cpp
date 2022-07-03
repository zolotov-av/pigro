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

PigroConsole::PigroConsole(QObject *parent): QObject(parent)
{
    connect(link, &PigroApp::started, this, &PigroConsole::pigroStarted);
    connect(link, &PigroApp::stopped, this, &PigroConsole::pigroStopped);
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
    link->checkProtoVersion();
    link->dumpProtoVersion();
    link->execute(m_action);
    link->stop();
    link->wait();
    return 0;
}
