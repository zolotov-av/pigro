//
// Программатор микроконтроллеров AVR.
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

#include <QApplication>
#include "PigroApp.h"
#include "PigroWindow.h"

/**
* Отобразить подсказку
*/
int help()
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

int real_main(int argc, char *argv[])
{
	if ( argc <= 1 ) return help();
	
    PigroAction action;

    bool verbose = false;
    for(int i = 1; i < argc; i++)
    {
        if ( strcmp(argv[i], "-v") == 0 )
        {
            verbose = true;
        }
        else if ( strcmp(argv[i], "-q") == 0 )
        {
            verbose = false;
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
	

    PigroApp app("/dev/ttyUSB0", verbose);
    app.checkProtoVersion();
    app.dumpProtoVersion();
    app.execute(action);

    return 0;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("shamangrad");
    QCoreApplication::setOrganizationDomain("shamangrad.net");
    QCoreApplication::setApplicationName("pigro");

    constexpr bool gui = true;

    if constexpr ( gui )
    {
        PigroWindow w;
        w.show();

        return app.exec();
    }
    else
    {
        try
        {
            return real_main(argc, argv);
        }
        catch (const nano::exception &e)
        {
            std::cerr << "[nano::exception] " << e.message() << std::endl;
            return 1;
        }
        catch(const std::exception &e)
        {
            std::cerr << "[std::exception] " << e.what() << std::endl;
            return 1;
        }
    }
}
