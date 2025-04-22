//https://github.com/jhasse/poly2tri/tree/master/poly2tri
INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/common/dll_symbol.h \
    $$PWD/common/shapes.h \
    $$PWD/common/utils.h \
    $$PWD/sweep/advancing_front.h \
    $$PWD/sweep/cdt.h \
    $$PWD/sweep/sweep_context.h \
    $$PWD/sweep/sweep.h \
    $$PWD/poly2tri.h

SOURCES += \
    $$PWD/common/shapes.cc \
    $$PWD/sweep/advancing_front.cc \
    $$PWD/sweep/cdt.cc \
    $$PWD/sweep/sweep_context.cc \
    $$PWD/sweep/sweep.cc

