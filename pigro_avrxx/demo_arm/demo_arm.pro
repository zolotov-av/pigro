TEMPLATE = app
CONFIG += c++17
CONFIG -= app_bundle
CONFIG -= qt

DEFINES += STM32F100xB

HEADERS += \
        armxx.h \
        armxx/bitband.h \
        armxx/static_ptr.h \
        isr.h \
        tiny/io.h

SOURCES += \
        armxx.cpp \
        armxx/bitband.cpp \
        armxx/static_ptr.cpp \
        isr.cpp \
        isr_vector.cpp \
        main.cpp \
        tiny/io.cpp


INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

MCU = cortex-m3
LD_SCRIPT = STM32F100XB_FLASH.ld

QMAKE_CXXFLAGS += -mcpu=$$MCU -mthumb
QMAKE_LFLAGS += -T./$$LD_SCRIPT

QMAKE_POST_LINK = $$QMAKE_OBJCOPY -O ihex ${TARGET} ${TARGET}.hex;
QMAKE_POST_LINK += $$QMAKE_OBJDUMP -d ${TARGET} > ${TARGET}.dump

QMAKE_CLEAN += $${TARGET}.hex $${TARGET}.dump

QMAKE_CXXFLAGS += -Wa,-adhln=temp.s
