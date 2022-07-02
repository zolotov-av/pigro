#include "PigroApp.h"

PigroApp::PigroApp(const char *path, bool verbose): m_verbose(verbose), config(nano::IniReader<512>("pigro.ini").data)
{
    serial->setPortName(path);
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    if ( !serial->open(QIODevice::ReadWrite))
    {
        throw nano::exception(serial->errorString().toStdString());
    }
}

PigroApp::~PigroApp()
{
    if ( driver ) delete driver;
}
