TEMPLATE = app
TARGET = pigro-gui

QT += core gui widgets serialport

CONFIG += c++17 utf8_source

SOURCES += \
    PigroWindow.cpp \
    main_gui.cpp

include(../common.pri)

RESOURCES += \
    PigroGuiResources.qrc

FORMS += \
    PigroWindow.ui

HEADERS += \
    PigroWindow.h
