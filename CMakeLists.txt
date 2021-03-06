# KMidimon - ALSA Sequencer based MIDI Monitor
# Copyright (C) 2005-2021 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
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

cmake_minimum_required(VERSION 3.14)

project(kmidimon
    VERSION 1.0.1
    LANGUAGES CXX
    DESCRIPTION "ALSA Sequencer based MIDI Monitor"
    HOMEPAGE_URL "https://kmidimon.sourceforge.io/"
)

option(EMBED_TRANSLATIONS "Embed translations instead of installing" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_admin")
include(SCMRevision)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED Yes)

find_package(Qt5 5.12 COMPONENTS Widgets LinguistTools REQUIRED)
if(Qt5_FOUND)
    message(STATUS "Found Qt5 version: ${Qt5Core_VERSION_STRING}")
endif()

find_package(Drumstick 2.0 COMPONENTS ALSA File REQUIRED)
if(Drumstick_FOUND)
    message(STATUS "Found Drumstick version: ${Drumstick_VERSION}")
endif()

message (STATUS "${PROJECT_NAME} v${PROJECT_VERSION}
    install prefix: ${CMAKE_INSTALL_PREFIX}
    Build configuration: ${CMAKE_BUILD_TYPE}
    Processor: ${CMAKE_SYSTEM_PROCESSOR}
    Qt5: ${Qt5_VERSION}
    Drumstick: ${Drumstick_VERSION}
    Embed translations: ${EMBED_TRANSLATIONS}")

include(GNUInstallDirs)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wzero-as-null-pointer-constant -Wsuggest-override")
endif()

add_subdirectory(icons)
add_subdirectory(doc/en)
if(NOT EMBED_TRANSLATIONS)
    add_subdirectory(translations)
endif()
add_subdirectory(src)

configure_file(
    "${CMAKE_SOURCE_DIR}/kmidimon.spec.in"
    "${CMAKE_SOURCE_DIR}/kmidimon.spec"
    IMMEDIATE @ONLY)

configure_file(
    "${CMAKE_SOURCE_DIR}/cmake_admin/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target ( uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

add_custom_target ( tarball
    COMMAND mkdir -p kmidimon-${PROJECT_VERSION}
    COMMAND cp -r cmake_admin kmidimon-${PROJECT_VERSION}
    COMMAND cp -r doc kmidimon-${PROJECT_VERSION}
    COMMAND cp -r icons kmidimon-${PROJECT_VERSION}
    COMMAND cp -r src kmidimon-${PROJECT_VERSION}
    COMMAND cp -r translations kmidimon-${PROJECT_VERSION}
    COMMAND cp CMakeLists.txt AUTHORS COPYING ChangeLog INSTALL NEWS README TODO *.ins *.spec* *.desktop *.lsm *.qrc *.pro *.pri kmidimon-${PROJECT_VERSION}
    COMMAND tar -cj --exclude .svn --exclude *.old --exclude *~ -f kmidimon-${PROJECT_VERSION}.tar.bz2 kmidimon-${PROJECT_VERSION}
    COMMAND tar -cz --exclude .svn --exclude *.old --exclude *~ -f kmidimon-${PROJECT_VERSION}.tar.gz kmidimon-${PROJECT_VERSION}
    COMMAND rm -rf kmidimon-${PROJECT_VERSION}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

install( FILES net.sourceforge.kmidimon.desktop
         DESTINATION ${CMAKE_INSTALL_DATADIR}/applications )
install( FILES net.sourceforge.kmidimon.appdata.xml
         DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo )

install( FILES standards.ins
         DESTINATION ${CMAKE_INSTALL_DATADIR}/kmidimon )
