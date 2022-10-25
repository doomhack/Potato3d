QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += force_debug_info


# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += _USE_MATH_DEFINES

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../3dmaths/recip.cpp \
    ../RenderDevice.cpp \
    ../RenderTarget.cpp \
    ../bspmodel.cpp \
    ../object3d.cpp \
    bsp3d.cpp \
    main.cpp \
    mainwindow.cpp \
    mainwindow2.cpp

HEADERS += \
    ../3dmaths/divide.h \
    ../3dmaths/f3dmath.h \
    ../3dmaths/fp.h \
    ../3dmaths/m4.h \
    ../3dmaths/recip.h \
    ../3dmaths/utils.h \
    ../3dmaths/v2.h \
    ../3dmaths/v3.h \
    ../3dmaths/v4.h \
    ../RenderCommon.h \
    ../RenderDevice.h \
    ../RenderTarget.h \
    ../RenderTriangle.h \
    ../TextureCache.h \
    ../bspmodel.h \
    ../common.h \
    ../object3d.h \
    ../potato3d.h \
    ../rtypes.h \
    bsp3d.h \
    mainwindow.h \
    mainwindow2.h \
    models/model.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resources.qrc

#LIBS += $$PWD/codeprophet.lib
#QMAKE_CXXFLAGS += /GH /Gh
#QMAKE_CFLAGS += /GH /Gh

QMAKE_CXXFLAGS += /GL
QMAKE_CFLAGS += /GL

QMAKE_LFLAGS += /LTCG
