# KMidimon - ALSA Sequencer based MIDI Monitor
# Copyright (C) 2005-2020 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
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
# along with this program. If not, see <http://www.gnu.org/licenses/>.

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(EMBED_TRANSLATIONS)
    set( TRANSLATIONS
         ../translations/kmidimon_cs.ts
         ../translations/kmidimon_es.ts
         ../translations/kmidimon_fr.ts
         ../translations/kmidimon_ja.ts
    )
    qt5_add_translation(QM_FILES ${TRANSLATIONS})
    include(TranslationUtils)
    add_app_translations_resource(APP_RES ${QM_FILES})
    add_qt_translations_resource(QT_RES cs es fr ja)
endif()

set(kmidimon_SRCS
    about.cpp
    about.h
    configdialog.cpp
    configdialog.h
    connectdlg.cpp
    connectdlg.h
    eventfilter.cpp
    eventfilter.h
    helpwindow.cpp
    helpwindow.h
    iconutils.cpp
    iconutils.h
    instrument.cpp
    instrument.h
    kmidimon.cpp
    kmidimon.h
    main.cpp
    player.cpp
    player.h
    proxymodel.cpp
    proxymodel.h
    sequenceradaptor.cpp
    sequenceradaptor.h
    sequenceitem.cpp
    sequenceitem.h
    sequencemodel.cpp
    sequencemodel.h
    slideraction.cpp
    slideraction.h
    about.ui
    configdialogbase.ui
    kmidimonwin.ui
    kmidimon.qrc
    ${APP_RES}
    ${QT_RES}
)

add_executable(kmidimon ${kmidimon_SRCS})

target_link_libraries( kmidimon
    Qt5::Widgets
    Drumstick::ALSA
    Drumstick::File
)

target_compile_definitions( kmidimon PRIVATE
    -DVERSION=${PROJECT_VERSION}
    -DTRANSLATIONS_PATH=$<IF:$<BOOL:${EMBED_TRANSLATIONS}>,:/,${CMAKE_INSTALL_DATAROOTDIR}/kmidimon/translations/>
)

install( TARGETS kmidimon
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
