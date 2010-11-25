#-------------------------------------------------
#
# Project created by QtCreator 2010-11-23T18:43:38
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_qtlambdatest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += .. ../../boost_1_45_0

SOURCES += tst_qtlambdatest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

LIBS += -L../Debug -L../Release -lQtLambda
