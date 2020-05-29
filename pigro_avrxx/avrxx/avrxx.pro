TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
CONFIG += c++17

SOURCES += \
    avrxx.cpp \
    iobuf.cpp \
    ringbuf.cpp \
    spibuf.cpp \
    uart.cpp \
    uartbuf.cpp

HEADERS += \
    avrxx.h \
    iobuf.h \
    ringbuf.h \
    spibuf.h \
    uart.h \
    uartbuf.h

AVR_MCU = atmega16a

QMAKE_CXXFLAGS += -mmcu=$$AVR_MCU
QMAKE_LFLAGS += -mmcu=$$AVR_MCU
