TEMPLATE = app

QT += core gui widgets serialport

CONFIG += c++17
#CONFIG -= app_bundle
#CONFIG -= qt

SOURCES += \
    ARM.cpp \
    AVR.cpp \
    DeviceInfo.cpp \
    FirmwareData.cpp \
    PigroApp.cpp \
    PigroDriver.cpp \
    PigroLink.cpp \
    PigroWindow.cpp \
    nano/config.cpp \
    nano/ini.cpp \
    nano/map.cpp \
    nano/math.cpp \
    nano/string.cpp \
    nano/IniReader.cpp \
    nano/TextReader.cpp \
    nano/exception.cpp \
    IntelHEX.cpp \
    main.cpp

HEADERS += \
    ARM.h \
    AVR.h \
    DeviceInfo.h \
    FirmwareData.h \
    PigroApp.h \
    PigroDriver.h \
    PigroLink.h \
    PigroWindow.h \
    nano/config.h \
    nano/ini.h \
    nano/map.h \
    nano/math.h \
    nano/string.h \
    nano/IniReader.h \
    nano/TextReader.h \
    nano/exception.h \
    IntelHEX.h

FORMS += \
    PigroWindow.ui

RESOURCES += \
    PigroResources.qrc
