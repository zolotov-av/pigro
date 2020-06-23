TEMPLATE = app
CONFIG += c++17
CONFIG -= app_bundle
CONFIG -= qt

HEADERS += \
    PigroProto.h \
    PigroService.h \
    PigroTimer.h \
    jtag.h \
    stm32.h

SOURCES += \
        main.cpp


INCLUDEPATH += $$PWD/../avrxx $$PWD/../libtiny
DEPENDPATH += $$PWD/../avrxx $$PWD/../libtiny

AVR_MCU = atmega16a

INLINE_LIMIT=40

QMAKE_CXXFLAGS += -mmcu=$$AVR_MCU -finline-limit=$$INLINE_LIMIT
QMAKE_LFLAGS += -mmcu=$$AVR_MCU

QMAKE_POST_LINK = avr-objcopy -O ihex ${TARGET} ${TARGET}.hex;
QMAKE_POST_LINK += avr-objdump -d ${TARGET} | avr-c++filt > ${TARGET}.dump

QMAKE_CLEAN += $${TARGET}.hex $${TARGET}.dump
