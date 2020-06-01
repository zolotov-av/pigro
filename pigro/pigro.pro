TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        IntelHEX.cpp \
        main.cpp \
        serial.cpp

HEADERS += \
    IntelHEX.h \
    serial.h
