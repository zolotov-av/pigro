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


#include <QApplication>
#include "PigroWindow.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("pigro-gui");
    QCoreApplication::setApplicationVersion("0.1");
    QCoreApplication::setOrganizationDomain("shamangrad.net");
    QCoreApplication::setOrganizationName("shamangrad");

    PigroWindow w;
    w.show();

    return app.exec();
}
