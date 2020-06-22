TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt


INCLUDEPATH += "include"

INCLUDEPATH += "C:\devkitPro\libgba\include"

SOURCES += \
    source/main.cpp \
    source/model.cpp \
    source/object3d.cpp \
    source/render.iwram.cpp


DISTFILES += \
    Makefile

HEADERS += \
    include/common.h \
    include/f3dmath.h \
    include/fp.h \
    include/m4.h \
    include/model.h \
    include/object3d.h \
    include/potato3d.h \
    include/render.h \
    include/rtypes.h \
    include/utils.h \
    include/v2.h \
    include/v3.h \
    include/v4.h
