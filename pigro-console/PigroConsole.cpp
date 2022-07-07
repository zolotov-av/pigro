#include "PigroConsole.h"
#include <pigro/trace.h>


void PigroConsole::execute(PigroAction action)
{
    switch ( action )
    {
    case AT_ACT_INFO: return pigro->action_info();
    case AT_ACT_STAT: return pigro->action_stat();
    case AT_ACT_CHECK: return pigro->action_check();
    case AT_ACT_WRITE: return pigro->action_write();
    case AT_ACT_ERASE: return pigro->action_erase();
    case AT_ACT_READ_FUSE: return pigro->action_read_fuse();
    case AT_ACT_WRITE_FUSE: return pigro->action_write_fuse();
    case AT_ACT_TEST: return pigro->action_test();
    default: throw nano::exception("Victory!");
    }
}

void PigroConsole::sessionStarted(int major, int minor)
{
    printf("session started, protocol version: %d.%d\n", major, minor);
}

void PigroConsole::sessionStopped()
{
    printf("session stopped\n");
}

void PigroConsole::reportMessage(const QString &message)
{
    const std::string msg = message.toStdString();
    printf("%s\n", msg.c_str());
}

PigroConsole::PigroConsole(QObject *parent): QObject(parent)
{
    connect(pigro, &Pigro::sessionStarted, this, &PigroConsole::sessionStarted);
    connect(pigro, &Pigro::sessionStopped, this, &PigroConsole::sessionStopped);
    connect(pigro, &Pigro::reportMessage, this, &PigroConsole::reportMessage);
}

PigroConsole::~PigroConsole()
{
}

int PigroConsole::exec(PigroAction action)
{
    m_action = action;

    pigro->openProject("pigro.ini");
    pigro->openSerialPort("/dev/ttyUSB0");
    printf("\n--- BEGIN ---\n\n");
    try
    {
        execute(m_action);
    }
    catch (const std::exception &e)
    {
        printf("[ FAIL ]: %s", e.what());
    }

    printf("\n--- END ---\n\n");
    pigro->closeSerialPort();

    return 0;
}
