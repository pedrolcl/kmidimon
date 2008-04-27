# KMidimon - ALSA Sequencer based MIDI Monitor
# Copyright (C) 2005-2008 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
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
    ${KDE3_LIB_DIR} 
    ${ALSA_LIB_DIR}
)

INCLUDE_DIRECTORIES(AFTER
    ${QT_INCLUDE_DIR} 
    ${KDE3_INCLUDE_DIR} 
    ${ALSA_INC_DIR}
)

SET(kmidimon_SRCS 
	configdialog.cpp
	configdialog.h
	connectdlg.cpp
	connectdlg.h
	debugdef.h
	fancylistviewitem.cpp
	fancylistviewitem.h
	kmidimon.cpp
	kmidimon.h
	kmidimonwidget.cpp
	kmidimonwidget.h
	main.cpp
	sequencerclient.cpp
	sequencerclient.h
	qt_kde.h
)

SET(kmidimon_UI 
	configdialogbase.ui
	kmidimonwidgetbase.ui
)

#SET(kmidimon_DCOP "")
#KDE3_ADD_DCOP_SKELS(kmidimon_SRCS ${kmidimon_DCOP})
#KDE3_ADD_DCOP_STUBS(kmidimon_SRCS ${kmidimon_DCOP}) 

KDE3_ADD_UI_FILES(kmidimon_SRCS ${kmidimon_UI})
KDE3_AUTOMOC(${kmidimon_SRCS})
KDE3_ADD_EXECUTABLE(kmidimon ${kmidimon_SRCS})

IF(USE_PCH)
    ADD_DEPENDENCIES(kmidimon precompiled_headers)
ENDIF(USE_PCH)

TARGET_LINK_LIBRARIES( kmidimon
    ${QT_AND_KDECORE_LIBS} 
    ${ALSA_LIBS} 
    kdeui
    kio
)

# Install targets
INSTALL(TARGETS kmidimon
        RUNTIME DESTINATION ${KDE3EXECDIR})

# Install GUI resource files
FILE(GLOB RC_FILES *.rc)
INSTALL(FILES ${RC_FILES}
        DESTINATION ${KDE3DATADIR}/kmidimon)

# Install icons
INSTALL(FILES icons/hi16-app-kmidimon.png
        DESTINATION ${KDE3ICONDIR}/hicolor/16x16/apps   
        RENAME kmidimon.png)
INSTALL(FILES icons/hi32-app-kmidimon.png
        DESTINATION ${KDE3ICONDIR}/hicolor/32x32/apps   
        RENAME kmidimon.png)
INSTALL(FILES icons/hi16-action-kmidimon_record.png
	DESTINATION ${KDE3ICONDIR}/hicolor/16x16/actions        
        RENAME kmidimon_record.png)
INSTALL(FILES icons/hi22-action-kmidimon_record.png
	DESTINATION ${KDE3ICONDIR}/hicolor/22x22/actions        
        RENAME kmidimon_record.png)
INSTALL(FILES icons/hi32-action-kmidimon_record.png
	DESTINATION ${KDE3ICONDIR}/hicolor/32x32/actions        
        RENAME kmidimon_record.png)
INSTALL(FILES kmidimon.desktop
	DESTINATION ${KDE3MENUDIR})
