TEMPLATE = app
CONFIG += c++17
CONFIG -= app_bundle
CONFIG -= qt

DEFINES += STM32F100xB

HEADERS += \
        armxx/bitband.h \
        armxx/pin.h \
        armxx/static_ptr.h \
        isr.h \
        tiny/io.h \
        tiny/iobuf.h \
        tiny/ringbuf.h \
        tiny/system.h \
        tiny/system_arm.h \
        tiny/uartbuf.h \
        uart.h

SOURCES += \
        armxx/bitband.cpp \
        armxx/pin.cpp \
        armxx/static_ptr.cpp \
        isr.cpp \
        isr_vector.cpp \
        main.cpp \
        tiny/io.cpp \
        tiny/iobuf.cpp \
        tiny/ringbuf.cpp \
        tiny/system.cpp \
        tiny/system_arm.cpp \
        tiny/uartbuf.cpp \
        uart.cpp


INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

MCU = cortex-m3
LD_SCRIPT = STM32F100XB_FLASH.ld

QMAKE_CXXFLAGS += -mcpu=$$MCU -mthumb -ffreestanding
QMAKE_LFLAGS += -T./$$LD_SCRIPT

QMAKE_POST_LINK = $$QMAKE_OBJCOPY -O ihex ${TARGET} ${TARGET}.hex;
QMAKE_POST_LINK += $$QMAKE_OBJDUMP -d ${TARGET} > ${TARGET}.dump

QMAKE_CLEAN += $${TARGET}.hex $${TARGET}.dump

QMAKE_CXXFLAGS += -Wa,-adhln=temp.s
