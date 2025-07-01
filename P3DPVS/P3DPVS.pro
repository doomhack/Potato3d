QT += core gui widgets

CONFIG += c++20
CONFIG += force_debug_info

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += STORE_PVS

SOURCES += \
        source/bspmodel.redir.iwram.cpp \
        source/camera.cpp \
        source/collision.cpp \
        source/mainloop.iwram.cpp \
        source/model.cpp \
        source/main.cpp \
        source/recip.redir.cpp \
        source/setup.cpp \
        source/videosystem.cpp \
        source/worldmodel.cpp



HEADERS += \
    include/camera.h \
    include/common.h \
    include/mainloop.h \
    include/model.h \
    include/setup.h \
    include/videosystem.h \
    include/worldmodel.h \
    include/collision.h


    # Default rules for deployment.
    qnx: target.path = /tmp/$${TARGET}/bin
    else: unix:!android: target.path = /opt/$${TARGET}/bin
    !isEmpty(target.path): INSTALLS += target

QMAKE_CXXFLAGS += /GL
QMAKE_CFLAGS += /GL

QMAKE_LFLAGS += /LTCG
