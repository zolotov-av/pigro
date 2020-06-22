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
        uart.h

SOURCES += \
        armxx/bitband.cpp \
        armxx/pin.cpp \
        armxx/static_ptr.cpp \
        isr.cpp \
        isr_vector.cpp \
        main.cpp \
        uart.cpp


INCLUDEPATH += $$PWD/include $$PWD/../libtiny
DEPENDPATH += $$PWD/include $$PWD/../libtiny

MCU = cortex-m3
LD_SCRIPT = STM32F100XB_FLASH.ld

QMAKE_CXXFLAGS += -mcpu=$$MCU -mthumb -ffreestanding
QMAKE_LFLAGS += -T./$$LD_SCRIPT

QMAKE_POST_LINK = $$QMAKE_OBJCOPY -O ihex ${TARGET} ${TARGET}.hex;
QMAKE_POST_LINK += $$QMAKE_OBJDUMP -d ${TARGET} > ${TARGET}.dump

QMAKE_CLEAN += $${TARGET}.hex $${TARGET}.dump

QMAKE_CXXFLAGS += -Wa,-adhln=temp.s
