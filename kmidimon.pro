TEMPLATE = app
TARGET = kmidimon
DEPENDPATH += . src
INCLUDEPATH += . src
VERSION = 1.0.0
QT += core gui widgets

DEFINES += VERSION=$$VERSION

PKGCONFIG += alsa drumstick-alsa drumstick-file
CONFIG += link_pkgconfig

# The following define makes your compiler warn you if you use any
# feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
#DEFINES += QT_NO_VERSION_TAGGING

# Input
HEADERS += src/configdialog.h \
           src/connectdlg.h \
           src/eventfilter.h \
           src/instrument.h \
           src/kmidimon.h \
           src/player.h \
           src/proxymodel.h \
           src/sequenceitem.h \
           src/sequencemodel.h \
           src/sequenceradaptor.h \
           src/slideraction.h

FORMS   += src/configdialogbase.ui \
           src/kmidimonwin.ui

SOURCES += src/configdialog.cpp \
           src/connectdlg.cpp \
           src/eventfilter.cpp \
           src/instrument.cpp \
           src/kmidimon.cpp \
           src/main.cpp \
           src/player.cpp \
           src/proxymodel.cpp \
           src/sequenceitem.cpp \
           src/sequencemodel.cpp \
           src/sequenceradaptor.cpp \
           src/slideraction.cpp
