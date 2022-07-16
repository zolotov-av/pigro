
PIGRO_DIR=$$PWD $$PWD/pigro $$OUT_PWD/../pigro

INCLUDEPATH = $$PIGRO_DIR
DEPENDPATH = $$PIGRO_DIR

LIBS += -L$$OUT_PWD/../pigro -lpigro

PRE_TARGETDEPS += $$OUT_PWD/../pigro/libpigro.a
