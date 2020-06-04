TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    AVR.cpp \
    nano/string.cpp \
    nano/IniReader.cpp \
    nano/TextReader.cpp \
    nano/exception.cpp \
    nano/serial.cpp \
    IntelHEX.cpp \
    main.cpp

HEADERS += \
    AVR.h \
    nano/string.h \
    nano/IniReader.h \
    nano/TextReader.h \
    nano/exception.h \
    nano/serial.h \
    IntelHEX.h
