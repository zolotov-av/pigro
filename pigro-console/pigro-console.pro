TEMPLATE = app
TARGET = pigro

QT -= gui
QT += core serialport
#QT = core serialport

CONFIG += c++17 utf8_source cmdline

SOURCES += \
    PigroConsole.cpp \
    main_console.cpp

include(../common.pri)

HEADERS += \
    PigroConsole.h
