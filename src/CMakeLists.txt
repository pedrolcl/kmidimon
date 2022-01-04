# Drumstick MIDI Monitor
# Copyright (C) 2005-2022 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
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
    if(QT_VERSION VERSION_LESS 5.15.0)
        qt5_add_translation(QM_FILES ${TRANSLATIONS})
    else()
        qt_add_translation(QM_FILES ${TRANSLATIONS})
    endif()
    include(TranslationUtils)
    add_app_translations_resource(APP_RES ${QM_FILES})
    add_qt_translations_resource(QT_RES cs es fr ja)
endif()

find_package(ALSA 1.0.0 REQUIRED)

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
    ../datafiles.qrc
    ../doc/docs.qrc
    ${APP_RES}
    ${QT_RES}
)

add_executable(kmidimon ${kmidimon_SRCS})
if (BUILD_DOCS)
    add_dependencies ( kmidimon update_helpfiles )
endif()

target_link_libraries( kmidimon
    Qt${QT_VERSION_MAJOR}::Widgets
    Drumstick::File
    Drumstick::ALSA
    ALSA::ALSA
)

if (QT_VERSION VERSION_GREATER_EQUAL 6.0.0)
    target_link_libraries( kmidimon
        Qt6::Gui
        Qt6::Core5Compat
    )
endif()

target_compile_definitions( kmidimon PRIVATE
    VERSION=${PROJECT_VERSION}
    $<$<BOOL:${EMBED_TRANSLATIONS}>:TRANSLATIONS_EMBEDDED>
)

if (DEFINED PROJECT_WC_REVISION)
    target_compile_definitions( ${PROJECT_NAME} PRIVATE
        REVISION=${PROJECT_WC_REVISION} )
endif()

install( TARGETS kmidimon
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
