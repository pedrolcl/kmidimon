# KMidimon - ALSA Sequencer based MIDI Monitor
# Copyright (C) 2005-2009 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA

IF(NOT KMIDIMON_SOURCE_DIR)
    MESSAGE(FATAL_ERROR "You need to run cmake or ccmake at the root directory")
ENDIF(NOT KMIDIMON_SOURCE_DIR)

IF(USE_PCH)
    ADD_PRECOMPILED_HEADER(precompiled_headers qt_kde.h)
ENDIF(USE_PCH)

LINK_DIRECTORIES(
    ${ALSA_LIB_DIR}
)

SET(kmidimon_SRCS 
    configdialog.cpp
    configdialog.h
    connectdlg.cpp
    connectdlg.h
    kmidimon.cpp
    kmidimon.h
    main.cpp
    sequenceradaptor.cpp
    sequenceradaptor.h
    sequenceitem.cpp
    sequenceitem.h
    sequencemodel.cpp
    sequencemodel.h
)

SET(kmidimon_UI 
	configdialogbase.ui
)

KDE4_ADD_UI_FILES(kmidimon_SRCS ${kmidimon_UI})
KDE4_ADD_EXECUTABLE(kmidimon ${kmidimon_SRCS})

IF(USE_PCH)
    ADD_DEPENDENCIES(kmidimon precompiled_headers)
ENDIF(USE_PCH)

TARGET_LINK_LIBRARIES( kmidimon
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${ALSA_LIBS}
    aseqmm
)

INSTALL(TARGETS kmidimon RUNTIME DESTINATION ${BIN_INSTALL_DIR})
INSTALL(FILES kmidimonui.rc DESTINATION ${DATA_INSTALL_DIR}/kmidimon)
INSTALL(FILES icons/hi16-app-kmidimon.png
        DESTINATION ${ICON_INSTALL_DIR}/hicolor/16x16/apps   
        RENAME kmidimon.png)
INSTALL(FILES icons/hi32-app-kmidimon.png
        DESTINATION ${ICON_INSTALL_DIR}/hicolor/32x32/apps   
        RENAME kmidimon.png)
