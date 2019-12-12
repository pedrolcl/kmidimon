# KMidimon - ALSA Sequencer based MIDI Monitor
# Copyright (C) 2005-2019 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
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

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(kmidimon_SRCS
    about.cpp
    about.h
    configdialog.cpp
    configdialog.h
    connectdlg.cpp
    connectdlg.h
    eventfilter.cpp
    eventfilter.h
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
)

add_executable(kmidimon ${kmidimon_SRCS})

target_link_libraries( kmidimon
    Qt5::Widgets
    Drumstick::ALSA
    Drumstick::File
)

target_compile_definitions( kmidimon
    PRIVATE -DVERSION=${PROJECT_VERSION}
)

install( TARGETS kmidimon
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
