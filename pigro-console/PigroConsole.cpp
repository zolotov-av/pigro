#include "PigroConsole.h"
#include <pigro/trace.h>


int PigroConsole::execute(PigroAction action)
{
    switch ( action )
    {
    case AT_ACT_INFO: return link->action_info();
    case AT_ACT_STAT: return link->action_stat();
    case AT_ACT_CHECK: return link->action_check();
    case AT_ACT_WRITE: return link->action_write();
    case AT_ACT_ERASE: return link->action_erase();
    case AT_ACT_READ_FUSE: return link->action_read_fuse();
    case AT_ACT_WRITE_FUSE: return link->action_write_fuse();
    case AT_ACT_TEST: return link->action_test();
    default: throw nano::exception("Victory!");
    }
}

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
    execute(m_action);
    link->stop();
    link->wait();
    return 0;
}
