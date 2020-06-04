TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    nano/TextReader.cpp \
    nano/exception.cpp \
    nano/serial.cpp \
    IntelHEX.cpp \
    main.cpp \

HEADERS += \
    nano/TextReader.h \
    nano/exception.h \
    nano/serial.h \
    IntelHEX.h \
