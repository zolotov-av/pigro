//
// Программатор микроконтроллеров AVR и STM32
//
// Изначально был написан на Raspberry Pi и для Raspberry Pi и использовал
// шину GPIO на Raspberry Pi, к которой непосредственно подключался
// программируемый контроллер. Эта версия программатора использует внешений
// самодельный переходник USB <-> SPI и может запускаться теоретически на
// любой linux-системе.
//
// (c) Alex V. Zolotov <zolotov-alex@shamangrad.net>, 2013
//    Fill free to copy, to compile, to use, to redistribute etc on your own risk.
//

#include <QCoreApplication>
#include <pigro/trace.h>
#include <pigro/PigroApp.h>
#include "PigroConsole.h"
#include <cstdio>

/**
 * Отобразить подсказку
 */
static int help()
{
    printf("pigro :action: :filename: :verbose|quiet:\n");
    printf("  action:\n");
    printf("    info  - read chip info\n");
    printf("    stat  - read file and check stats\n");
    printf("    check - read file and compare with device\n");
    printf("    write - read file and write to device\n");
    printf("    erase - just erase chip\n");
    printf("    rfuse - read fuses\n");
    printf("    wfuse - write fuses from pigro.ini\n");
    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("pigro");
    QCoreApplication::setApplicationVersion("0.1");
    QCoreApplication::setOrganizationDomain("shamangrad.net");
    QCoreApplication::setOrganizationName("shamangrad");

    trace::setThreadName("main");

    PigroConsole pigro;

    if ( argc <= 1 ) return help();

    for(int i = 1; i < argc; i++)
    {
        if ( strcmp(argv[i], "-v") == 0 )
        {
            pigro.setVerbose(true);
        }
        else if ( strcmp(argv[i], "-q") == 0 )
        {
            pigro.setVerbose(false);
        }
    }

    const char *action_arg = nullptr;
    for(int i = 1; i < argc; i++)
    {
        if ( argv[i][0] != '-' )
        {
            action_arg = argv[i];
        }
    }

    if ( action_arg == nullptr ) return help();

    PigroAction action;
    if ( strcmp(action_arg, "info") == 0 ) action = AT_ACT_INFO;
    else if ( strcmp(action_arg, "stat") == 0 ) action = AT_ACT_STAT;
    else if ( strcmp(action_arg, "check") == 0 ) action = AT_ACT_CHECK;
    else if ( strcmp(action_arg, "write") == 0 ) action = AT_ACT_WRITE;
    else if ( strcmp(action_arg, "erase") == 0 ) action = AT_ACT_ERASE;
    else if ( strcmp(action_arg, "rfuse") == 0 ) action = AT_ACT_READ_FUSE;
    else if ( strcmp(action_arg, "wfuse") == 0 ) action = AT_ACT_WRITE_FUSE;
    else if ( strcmp(action_arg, "arm") == 0 ) action = AT_ACT_TEST;
    else if ( strcmp(action_arg, "test") == 0 ) action = AT_ACT_TEST;
    else return help();

    return pigro.exec(action);
}
