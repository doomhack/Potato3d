QT = core gui

CONFIG += c++20 cmdline

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        bspbuilder.cpp \
        bspmodelexport.cpp \
        main.cpp \
        objloader.cpp \
        wuquant.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ../BspModelDefs.h \
    bspbuilder.h \
    bspmodelexport.h \
    config.h \
    objloader.h \
    wuquant.h

RESOURCES += \
    resources.qrc
