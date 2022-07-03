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
    PigroApp.cpp \
    PigroDriver.cpp \
    PigroLink.cpp \
    PigroPrivate.cpp \
    nano/config.cpp \
    nano/ini.cpp \
    nano/map.cpp \
    nano/math.cpp \
    nano/string.cpp \
    nano/IniReader.cpp \
    nano/TextReader.cpp \
    nano/exception.cpp \
    IntelHEX.cpp \
    trace.cpp

HEADERS += \
    ARM.h \
    AVR.h \
    DeviceInfo.h \
    FirmwareData.h \
    FirmwareInfo.h \
    PigroApp.h \
    PigroDriver.h \
    PigroLink.h \
    PigroPrivate.h \
    nano/config.h \
    nano/ini.h \
    nano/map.h \
    nano/math.h \
    nano/string.h \
    nano/IniReader.h \
    nano/TextReader.h \
    nano/exception.h \
    IntelHEX.h \
    trace.h

FORMS +=

RESOURCES += \
    PigroResources.qrc
