TEMPLATE = app
CONFIG += console c++20
CONFIG -= app_bundle
CONFIG += force_debug_info
QT += core gui



INCLUDEPATH += "include"

INCLUDEPATH += "C:\devkitPro\libgba\include"

SOURCES += \
    source/RenderDevice.iwram.cpp \
    source/RenderTarget.cpp \
    source/main.iwram.cpp \
    source/recip.cpp



#QMAKE_CXXFLAGS += /GL
#QMAKE_CFLAGS += /GL

#QMAKE_LFLAGS += /LTCG

DISTFILES += \
    Makefile

