# KMidimon - ALSA Sequencer based MIDI Monitor
# Copyright (C) 2005-2010 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
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

PROJECT(KMIDIMON)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(CMAKE_USE_RELATIVE_PATHS OFF)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_admin")

SET (VERSION_MAJOR "0")
SET (VERSION_MINOR "7")
SET (VERSION_PATCH "4")
SET (VERSION_SUFFIX "svn")
SET (VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}${VERSION_SUFFIX})
ADD_DEFINITIONS(-D'VERSION="${VERSION}"')

OPTION(ENABLE_PCH       "Enable precompiled headers" OFF)
OPTION(STATIC_DRUMSTICK "Build drumstick static library" ON)

FIND_PACKAGE(Qt4 REQUIRED)
FIND_PACKAGE(KDE4 REQUIRED)
INCLUDE(KDE4Defaults)
INCLUDE_DIRECTORIES(${QT_INCLUDES} ${KDE4_INCLUDES})
STRING(REPLACE "-fno-exceptions" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
SET(LIB "lib${LIB_SUFFIX}")
SET(LIBRARY_OUTPUT_PATH "")
SET(EXECUTABLE_OUTPUT_PATH "")
SEPARATE_ARGUMENTS(KDE4_DEFINITIONS)
SEPARATE_ARGUMENTS(QT_DEFINITIONS)
ADD_DEFINITIONS(${KDE4_DEFINITIONS})
ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS(-DQT_STRICT_ITERATORS)

MESSAGE(STATUS "Build configuration: ${CMAKE_BUILD_TYPE}")

FIND_PACKAGE(MSGFMT REQUIRED)
IF(MSGFMT_FOUND)
    MESSAGE(STATUS "Program msgfmt found (${MSGFMT_EXECUTABLE})")
ELSE(MSGFMT_FOUND)
    MESSAGE(FATAL_ERROR "Please install the program msgfmt (gettext)")
ENDIF(MSGFMT_FOUND)

FIND_PACKAGE(PkgConfig REQUIRED)
IF(PKG_CONFIG_FOUND)
    MESSAGE(STATUS "Program pkg-config found (${PKG_CONFIG_EXECUTABLE})")
ELSE(PKG_CONFIG_FOUND)
    MESSAGE(FATAL_ERROR "Please install pkg-config")
ENDIF(PKG_CONFIG_FOUND)

SET(HAVE_ALSA FALSE)
PKG_CHECK_MODULES(ALSA REQUIRED alsa>=1.0)
IF(ALSA_FOUND)
    SET(HAVE_ALSA TRUE)
    ADD_DEFINITIONS(-DHAVE_ALSA)
    SET(ALSA_LIBS ${ALSA_LIBRARIES})
    #MESSAGE("ALSA_LIBS: ${ALSA_LIBS}")
    LIST(APPEND ALSA_LIB_DIR ${ALSA_LIBRARY_DIRS} ${ALSA_LIBDIR} )
    #MESSAGE("ALSA_LIB_DIR: ${ALSA_LIB_DIR}")
    LIST(APPEND ALSA_INC_DIR ${ALSA_INCLUDE_DIRS} ${ALSA_INCLUDEDIR})
    #MESSAGE("ALSA_INC_DIR: ${ALSA_INC_DIR}")
ELSE(ALSA_FOUND)
    MESSAGE(FATAL_ERROR "Please install ALSA library and headers (libasound2)")
ENDIF(ALSA_FOUND)

# DRUMSTICK LIBRARY BUILD TYPE
IF(STATIC_DRUMSTICK)
    SET(BUILD_SHARED_LIBS OFF)
    SET(DRUMSTICK_LIBS drumstick-file drumstick-alsa)
    SET(DRUMSTICK_LIB_DIR ${KMIDIMON_BINARY_DIR})
    SET(DRUMSTICK_INC_DIR ${KMIDIMON_SOURCE_DIR}/drumstick/include)
    ADD_SUBDIRECTORY(drumstick)
ELSE(STATIC_DRUMSTICK)
    SET(BUILD_SHARED_LIBS ON)
    PKG_CHECK_MODULES(DRUMSTICK-FILE REQUIRED drumstick-file>=0.3)
    PKG_CHECK_MODULES(DRUMSTICK-ALSA REQUIRED drumstick-alsa>=0.3)
    IF(DRUMSTICK-FILE_FOUND AND DRUMSTICK-ALSA_FOUND)
       SET( DRUMSTICK_LIBS ${DRUMSTICK-FILE_LIBRARIES} 
                           ${DRUMSTICK-ALSA_LIBRARIES})
       #MESSAGE(STATUS "DRUMSTICK_LIBS: ${DRUMSTICK_LIBS}")
       LIST( APPEND DRUMSTICK_LIB_DIR 
          ${DRUMSTICK-FILE_LIBRARY_DIRS} ${DRUMSTICK-FILE_LIBDIR}
          ${DRUMSTICK-ALSA_LIBRARY_DIRS} ${DRUMSTICK-ALSA_LIBDIR} )
       #MESSAGE(STATUS "DRUMSTICK_LIB_DIR: ${DRUMSTICK_LIB_DIR}")
       LIST( APPEND DRUMSTICK_INC_DIR 
             ${DRUMSTICK-FILE_INCLUDE_DIRS} ${DRUMSTICK-FILE_INCLUDEDIR}
             ${DRUMSTICK-ALSA_INCLUDE_DIRS} ${DRUMSTICK-ALSA_INCLUDEDIR} )
       #MESSAGE(STATUS "DRUMSTICK_INC_DIR: ${DRUMSTICK_INC_DIR}")
    ELSE(DRUMSTICK-FILE_FOUND AND DRUMSTICK-ALSA_FOUND)
       MESSAGE(FATAL_ERROR "Please install drumstick libraries and headers" 
                           " (libdrumstick-file0, libdrumstick-alsa0)" )
    ENDIF(DRUMSTICK-FILE_FOUND AND DRUMSTICK-ALSA_FOUND)
ENDIF(STATIC_DRUMSTICK)

SET(USE_PCH FALSE)
IF(ENABLE_PCH)
    FIND_PACKAGE(PCHSupport)
    IF(PCHSupport_FOUND)
        SET(USE_PCH TRUE)
        MESSAGE(STATUS "Enabling precompiled headers for GCC ${gcc_compiler_version}") 
    ENDIF(PCHSupport_FOUND)
ENDIF(ENABLE_PCH)

MESSAGE(STATUS "kmidimon ${VERSION} install prefix: ${CMAKE_INSTALL_PREFIX}")

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(icons)
ADD_SUBDIRECTORY(po)
ADD_SUBDIRECTORY(doc)

CONFIGURE_FILE(
    "${CMAKE_SOURCE_DIR}/kmidimon.spec.in"
    "${CMAKE_SOURCE_DIR}/kmidimon.spec"
    IMMEDIATE @ONLY)

CONFIGURE_FILE(
    "${CMAKE_SOURCE_DIR}/cmake_admin/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

ADD_CUSTOM_TARGET ( tarball
    COMMAND mkdir -p kmidimon-${VERSION}
    COMMAND cp -r cmake_admin kmidimon-${VERSION}
    COMMAND cp -r doc kmidimon-${VERSION}
    COMMAND cp -r po kmidimon-${VERSION}
    COMMAND cp -r icons kmidimon-${VERSION}
    COMMAND cp -r src kmidimon-${VERSION}
    COMMAND cp -rHL drumstick kmidimon-${VERSION}
    COMMAND cp CMakeLists.txt AUTHORS COPYING ChangeLog INSTALL NEWS README TODO *.ins *.spec* *.desktop *.lsm kmidimon-${VERSION}
    COMMAND tar -cj --exclude .svn --exclude *.old --exclude *~ -f kmidimon-${VERSION}.tar.bz2 kmidimon-${VERSION}
    COMMAND tar -cz --exclude .svn --exclude *.old --exclude *~ -f kmidimon-${VERSION}.tar.gz kmidimon-${VERSION}
    COMMAND rm -rf kmidimon-${VERSION}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

INSTALL( FILES kmidimon.desktop
         DESTINATION ${XDG_APPS_INSTALL_DIR} )
         
INSTALL( FILES standards.ins 
         DESTINATION ${DATA_INSTALL_DIR}/kmidimon )
