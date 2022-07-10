TEMPLATE = app
TARGET = pigro-gui

QT += core gui widgets serialport

CONFIG += c++17 utf8_source

SOURCES += \
    PigroWindow.cpp \
    ProjectModel.cpp \
    main_gui.cpp

include(../common.pri)

RESOURCES += \
    PigroGuiResources.qrc

FORMS += \
    PigroWindow.ui

HEADERS += \
    PigroWindow.h \
    ProjectModel.h

DISTFILES += \
    pigro-gui/icons/icons8-cancel.png \
    pigro-gui/icons/icons8-compare.png \
    pigro-gui/icons/icons8-erase.png \
    pigro-gui/icons/icons8-export-csv.png \
    pigro-gui/icons/icons8-info.png \
    pigro-gui/icons/icons8-info.svg \
    pigro-gui/icons/icons8-write.png
