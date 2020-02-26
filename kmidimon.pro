TEMPLATE = app
TARGET = kmidimon
DEPENDPATH += . src
INCLUDEPATH += . src
VERSION = 1.0.0
QT += core gui widgets
CONFIG += c++11 link_pkgconfig lrelease embed_translations
DEFINES += VERSION=$$VERSION \
    TRANSLATIONS_PATH=':/'
LRELEASE_DIR='.'
QM_FILES_RESOURCE_PREFIX='/'

packagesExist(alsa) {
    PKGCONFIG += alsa
}

packagesExist(drumstick-alsa drumstick-file) {
    PKGCONFIG += drumstick-alsa drumstick-file
} else {
    INCLUDEPATH += $$(DRUMSTICKINCLUDES)
    LIBS += -L$$(DRUMSTICKLIBS) -ldrumstick-alsa -ldrumstick-file
}

DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += \
    src/configdialog.h \
    src/connectdlg.h \
    src/eventfilter.h \
    src/helpwindow.h \
    src/iconutils.h \
    src/instrument.h \
    src/kmidimon.h \
    src/player.h \
    src/proxymodel.h \
    src/sequenceitem.h \
    src/sequencemodel.h \
    src/sequenceradaptor.h \
    src/slideraction.h \
    src/about.h

FORMS += \
    src/configdialogbase.ui \
    src/kmidimonwin.ui \
    src/about.ui

SOURCES += \
    src/configdialog.cpp \
    src/connectdlg.cpp \
    src/eventfilter.cpp \
    src/helpwindow.cpp \
    src/iconutils.cpp \
    src/instrument.cpp \
    src/kmidimon.cpp \
    src/main.cpp \
    src/player.cpp \
    src/proxymodel.cpp \
    src/sequenceitem.cpp \
    src/sequencemodel.cpp \
    src/sequenceradaptor.cpp \
    src/slideraction.cpp \
    src/about.cpp

RESOURCES += \
    datafiles.qrc \
    doc/docs.qrc \
    src/kmidimon.qrc

TRANSLATIONS += \
    translations/kmidimon_cs.ts \
    translations/kmidimon_es.ts \
    translations/kmidimon_fr.ts \
    translations/kmidimon_ja.ts

LCONVERT_LANGS=cs es fr ja
include(lconvert.pri)
