TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
CONFIG += c++17

SOURCES += \
        tiny/io.cpp \
        tiny/iobuf.cpp \
        tiny/ringbuf.cpp \
        tiny/system.cpp \
        tiny/system_arm.cpp \
        tiny/system_avr.cpp \
        tiny/uartbuf.cpp

HEADERS += \
    tiny/io.h \
    tiny/iobuf.h \
    tiny/ringbuf.h \
    tiny/system.h \
    tiny/system_arm.h \
    tiny/system_avr.h \
    tiny/uartbuf.h


AVR_MCU = atmega16a

QMAKE_CXXFLAGS += -mmcu=$$AVR_MCU
QMAKE_LFLAGS += -mmcu=$$AVR_MCU
