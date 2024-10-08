TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG -= qt


INCLUDEPATH += "include"

INCLUDEPATH += "C:\devkitPro\libgba\include"

SOURCES += \
    source/bspmodel.cpp \
    source/main.cpp \
    source/model.cpp \
    source/object3d.cpp \
    source/recip.cpp \
    source/render.iwram.cpp


HEADERS += \
    ../3dmaths/divide.h \
    ../3dmaths/f3dmath.h \
    ../3dmaths/fp.h \
    ../3dmaths/m4.h \
    ../3dmaths/utils.h \
    ../3dmaths/v2.h \
    ../3dmaths/v3.h \
    ../3dmaths/v4.h \
    ../common.h \
    ../object3d.h \
    ../potato3d.h \
    ../render.h \
    ../rtypes.h \
    include/common.h \
    include/model.h

DISTFILES += \
    Makefile \
    source/fixeddiv.s


