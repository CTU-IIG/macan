#-------------------------------------------------
#
# Project created by QtCreator 2015-03-03T12:54:36
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = ControlPanel
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qcustomplot.cpp \
    macanconnection.cpp \
    virtualbutton.cpp \
    virtualleds.cpp \
    graph.cpp \
    macanworker.cpp \
    ../macan_config.c \
    ../ltk_pc.c


HEADERS  += mainwindow.h \
    qcustomplot.h \
    macanconnection.h \
    virtualbutton.h \
    virtualleds.h \
    graph.h \
    ../macan_config.h \
    macanworker.h

INCLUDEPATH += $$PWD/../

LIBS += -L$$PWD/../../../build/linux/_compiled/lib/ -lmacan
LIBS += -lev
LIBS += -lnettle

FORMS    += mainwindow.ui


INCLUDEPATH += $$PWD/../../../build/linux/_compiled/include
DEPENDPATH += $$PWD/../../../build/linux/_compiled/include

unix:!macx: PRE_TARGETDEPS += $$PWD/../../../build/linux/_compiled/lib/libmacan.a
