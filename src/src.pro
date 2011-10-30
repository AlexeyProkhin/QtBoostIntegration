include(../config.pri)

QT       -= gui
QMAKE_CXXFLAGS += --std=c++0x

TARGET = QtBoostIntegration

TEMPLATE = lib
CONFIG += staticlib

DEFINES += QTBOOSTINTEGRATION_LIBRARY_BUILD
DEFINES += QTBOOSTINTEGRATION_CHECK_SIGNATURE

SOURCES += \
    qtboostintegration.cpp \
    qtboostintegrationbindingobject.cpp

HEADERS += \
    QtBoostIntegration \
    qtboostintegration.h\
    qtboostintegrationbindingobject_p.h

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
    .gitignore \
    ../LICENSE.txt \
    ../README.txt
