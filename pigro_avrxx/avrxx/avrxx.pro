TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
CONFIG += c++17

SOURCES += \
    avrxx/io.cpp \
    avrxx/iobuf.cpp \
    avrxx/ringbuf.cpp \
    avrxx/spi.cpp \
    avrxx/spi_master.cpp \
    avrxx/uart.cpp \
    avrxx/uartbuf.cpp

HEADERS += \
    avrxx/io.h \
    avrxx/iobuf.h \
    avrxx/ringbuf.h \
    avrxx/spi.h \
    avrxx/spi_master.h \
    avrxx/uart.h \
    avrxx/uartbuf.h

AVR_MCU = atmega16a

QMAKE_CXXFLAGS += -mmcu=$$AVR_MCU
QMAKE_LFLAGS += -mmcu=$$AVR_MCU
