#-------------------------------------------------
#
# Project created by QtCreator 2010-11-22T16:32:18
#
#-------------------------------------------------

QT       -= gui

TARGET = QtLambda
TEMPLATE = lib

INCLUDEPATH += ../boost_1_45_0

DEFINES += QTLAMBDA_LIBRARY

SOURCES += qtlambda.cpp \
    bindingobject.cpp

HEADERS += qtlambda.h\
        QtLambda_global.h \
    bindingobject_p.h

symbian {
    #Symbian specific definitions
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE5DF9E41
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = QtLambda.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/local/lib
    }
    INSTALLS += target
}

OTHER_FILES += \
    .gitignore
