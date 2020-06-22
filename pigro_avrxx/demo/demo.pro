TEMPLATE = app
CONFIG += c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
        timer.cpp \
        tiny/jtag.cpp


INCLUDEPATH += $$PWD/../avrxx $$PWD/../libtiny
DEPENDPATH += $$PWD/../avrxx $$PWD/../libtiny

AVR_MCU = atmega16a

QMAKE_CXXFLAGS += -mmcu=$$AVR_MCU
QMAKE_LFLAGS += -mmcu=$$AVR_MCU

QMAKE_POST_LINK = avr-objcopy -O ihex ${TARGET} ${TARGET}.hex;
QMAKE_POST_LINK += avr-objdump -d ${TARGET} > ${TARGET}.dump

QMAKE_CLEAN += $${TARGET}.hex $${TARGET}.dump

QMAKE_CXXFLAGS += -Wa,-adhln=temp.s

HEADERS += \
    timer.h \
    tiny/jtag.h
