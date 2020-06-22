TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
CONFIG += c++17

SOURCES += \
    avrxx/io.cpp \
    avrxx/spi.cpp \
    avrxx/spi_master.cpp \
    avrxx/uart.cpp

HEADERS += \
    avrxx/io.h \
    avrxx/spi.h \
    avrxx/spi_master.h \
    avrxx/uart.h

INCLUDEPATH += $$PWD/../libtiny
DEPENDPATH += $$PWD/../libtiny

AVR_MCU = atmega16a

QMAKE_CXXFLAGS += -mmcu=$$AVR_MCU
QMAKE_LFLAGS += -mmcu=$$AVR_MCU
