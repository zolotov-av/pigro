TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    ARM.cpp \
    AVR.cpp \
    DeviceInfo.cpp \
    PigroDriver.cpp \
    PigroLink.cpp \
    nano/config.cpp \
    nano/ini.cpp \
    nano/map.cpp \
    nano/string.cpp \
    nano/IniReader.cpp \
    nano/TextReader.cpp \
    nano/exception.cpp \
    nano/serial.cpp \
    IntelHEX.cpp \
    main.cpp

HEADERS += \
    ARM.h \
    AVR.h \
    DeviceInfo.h \
    PigroDriver.h \
    PigroLink.h \
    nano/config.h \
    nano/ini.h \
    nano/map.h \
    nano/string.h \
    nano/IniReader.h \
    nano/TextReader.h \
    nano/exception.h \
    nano/serial.h \
    IntelHEX.h
