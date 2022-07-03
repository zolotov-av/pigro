TEMPLATE = app
TARGET = pigro

QT -= gui
QT += core serialport
#QT = core serialport

CONFIG += c++17 utf8_source cmdline

SOURCES += \
    main_console.cpp

include(../common.pri)
