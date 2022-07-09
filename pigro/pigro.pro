TEMPLATE = lib
CONFIG +=

QT += core gui widgets serialport

CONFIG += c++17 utf8_source staticlib

SOURCES += \
    ARM.cpp \
    AVR.cpp \
    DeviceInfo.cpp \
    FirmwareData.cpp \
    FirmwareInfo.cpp \
    Pigro.cpp \
    PigroApp.cpp \
    PigroDriver.cpp \
    PigroLink.cpp \
    nano/config.cpp \
    nano/ini.cpp \
    nano/map.cpp \
    nano/math.cpp \
    nano/string.cpp \
    nano/IniReader.cpp \
    nano/exception.cpp \
    IntelHEX.cpp \
    trace.cpp

HEADERS += \
    ARM.h \
    AVR.h \
    DeviceInfo.h \
    FirmwareData.h \
    FirmwareInfo.h \
    Pigro.h \
    PigroApp.h \
    PigroDriver.h \
    PigroLink.h \
    nano/config.h \
    nano/ini.h \
    nano/map.h \
    nano/math.h \
    nano/string.h \
    nano/IniReader.h \
    nano/exception.h \
    IntelHEX.h \
    trace.h

FORMS +=

RESOURCES += \
    PigroResources.qrc
