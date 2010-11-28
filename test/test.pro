include(../config.pri)

QT       += testlib
QT       -= gui

TARGET = qtboostintegrationtest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += .. ../../boost_1_44_0 ../src

SOURCES += qtboostintegrationtest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

LIBS += -L../src -lQtBoostIntegration
