include(../config.pri)

QT       += testlib
QT       -= gui

TARGET = qtboostintegrationtest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../src $$BOOST_DIR

SOURCES += qtboostintegrationtest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

LIBS += -L../src -lQtBoostIntegration
