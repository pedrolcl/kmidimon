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

PROJECT(KMIDIMON)

SET(CMAKE_USE_RELATIVE_PATHS OFF)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_admin")

SET (VERSION_MAJOR "0")
SET (VERSION_MINOR "7")
SET (VERSION_PATCH "1")
SET (VERSION_SUFFIX "cvs")
SET (VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}${VERSION_SUFFIX})
ADD_DEFINITIONS(-D'VERSION="${VERSION}"')

OPTION(WANT_PCH "Use precompiled headers" OFF)
OPTION(WANT_STATIC "Use aseqmm static library" ON)

FIND_PACKAGE(Qt4 REQUIRED)
FIND_PACKAGE(KDE4 REQUIRED)
INCLUDE(KDE4Defaults)
INCLUDE_DIRECTORIES ( 
    ${KDE4_INCLUDES} 
    ${QT_INCLUDES})
STRING(REPLACE "-fno-exceptions" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
SET(LIB "lib${LIB_SUFFIX}")
SET(LIBRARY_OUTPUT_PATH "")
SET(EXECUTABLE_OUTPUT_PATH "")
SEPARATE_ARGUMENTS(KDE4_DEFINITIONS)
SEPARATE_ARGUMENTS(QT_DEFINITIONS)
#MESSAGE(STATUS "KDE4 Definitions: ${KDE4_DEFINITIONS}") 
#MESSAGE(STATUS "QT Definitions: ${QT_DEFINITIONS}") 
ADD_DEFINITIONS(${KDE4_DEFINITIONS})
ADD_DEFINITIONS(${QT_DEFINITIONS})

MESSAGE(STATUS "Build configuration: ${CMAKE_BUILD_TYPE}")

# LIBRARY BUILD TYPE
IF(WANT_STATIC)
    SET(BUILD_SHARED_LIBS OFF)
ELSE(WANT_STATIC)
    SET(BUILD_SHARED_LIBS ON)
ENDIF(WANT_STATIC)

FIND_PACKAGE(MSGFMT REQUIRED)
IF(MSGFMT_FOUND)
    MESSAGE(STATUS "Program msgfmt found (${MSGFMT_EXECUTABLE})")
ENDIF(MSGFMT_FOUND)

FIND_PACKAGE(PkgConfig REQUIRED)
IF(PKG_CONFIG_FOUND)
    MESSAGE(STATUS "Program pkg-config found (${PKG_CONFIG_EXECUTABLE})")
ELSE(PKG_CONFIG_FOUND)
    MESSAGE(FATAL_ERROR "Program pkg-config not found")
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
    MESSAGE(FATAL_ERROR "Please install the required package. Aborting")
ENDIF(ALSA_FOUND)

#PKG_CHECK_MODULES(ASEQMM REQUIRED aseqmm)
#IF(ASEQMM_FOUND)
#   SET(ASEQMM_LIBS ${ASEQMM_LIBRARIES})
#   MESSAGE("ASEQMM_LIBS: ${ASEQMM_LIBS}")
#   LIST(APPEND ASEQMM_LIB_DIR ${ASEQMM_LIBRARY_DIRS} ${ASEQMM_LIBDIR} )
#   MESSAGE("ASEQMM_LIB_DIR: ${ASEQMM_LIB_DIR}")
#   LIST(APPEND ASEQMM_INC_DIR ${ASEQMM_INCLUDE_DIRS} ${ASEQMM_INCLUDEDIR})
#   MESSAGE("ASEQMM_INC_DIR: ${ASEQMM_INC_DIR}")
#ELSE(ASEQMM_FOUND)
#   MESSAGE(FATAL_ERROR "Please install the required package. Aborting")
#ENDIF(ASEQMM_FOUND)
SET(ASEQMM_LIBS aseqmm)
SET(ASEQMM_LIB_DIR ${KMIDIMON_BINARY_DIR})
SET(ASEQMM_INC_DIR ${KMIDIMON_SOURCE_DIR}/aseqmm/include)

SET(USE_PCH FALSE)
IF(WANT_PCH)
    FIND_PACKAGE(PCHSupport)
    IF(PCHSupport_FOUND)
        SET(USE_PCH TRUE)
        MESSAGE(STATUS "Enabling precompiled headers for GCC ${gcc_compiler_version}") 
    ENDIF(PCHSupport_FOUND)
ENDIF(WANT_PCH)

MESSAGE(STATUS "kmidimon ${VERSION} install prefix: ${CMAKE_INSTALL_PREFIX}")

ADD_SUBDIRECTORY(aseqmm build/aseqmm)
ADD_SUBDIRECTORY(src build)
ADD_SUBDIRECTORY(icons build/icons)
ADD_SUBDIRECTORY(po build/po)
ADD_SUBDIRECTORY(doc build/doc)

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

ADD_CUSTOM_TARGET ( distclean
    COMMAND rm -f  {,src/,po/,doc/}CMakeCache.txt
    COMMAND rm -f  {,src/,po/,doc/}CTestTestfile.cmake
    COMMAND rm -f  {,src/,po/,doc/}cmake_install.cmake
    COMMAND rm -f  {,src/,po/,doc/}cmake_uninstall.cmake
    COMMAND rm -f  {,src/,po/,doc/}CPackConfig.cmake
    COMMAND rm -f  {,src/,po/,doc/}CPackSourceConfig.cmake
    COMMAND rm -f  {,src/,po/,doc/}install_manifest.txt
    COMMAND rm -f  {,src/,po/,doc/}progress.make
    COMMAND rm -f  {,src/,po/,doc/}kmidimon.spec
    COMMAND rm -f  {,src/,po/,doc/}Makefile
    COMMAND rm -rf {,src/,po/,doc/}CMakeFiles
    COMMAND rm -rf {,src/,po/,doc/}CMakeTmp
    COMMAND rm -rf {,src/,po/,doc/}_CPack_Packages
    COMMAND rm -rf build/*
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
ADD_DEPENDENCIES(distclean clean)

ADD_CUSTOM_TARGET ( tarball
    COMMAND mkdir -p kmidimon-${VERSION}
    COMMAND cp -r cmake_admin kmidimon-${VERSION}
    COMMAND cp -r doc kmidimon-${VERSION}
    COMMAND cp -r po kmidimon-${VERSION}
    COMMAND cp -r icons kmidimon-${VERSION}
    COMMAND cp -r src kmidimon-${VERSION}
    COMMAND cp -rHL aseqmm kmidimon-${VERSION}
    COMMAND cp CMakeLists.txt AUTHORS COPYING ChangeLog INSTALL NEWS README TODO *.ins *.spec* *.desktop *.lsm kmidimon-${VERSION}
    COMMAND tar -cj --exclude CVS --exclude .cvsignore --exclude *.old --exclude *~ -f kmidimon-${VERSION}.tar.bz2 kmidimon-${VERSION}
    COMMAND tar -cz --exclude CVS --exclude .cvsignore --exclude *.old --exclude *~ -f kmidimon-${VERSION}.tar.gz kmidimon-${VERSION}
    COMMAND rm -rf kmidimon-${VERSION}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

INSTALL( FILES kmidimon.desktop
         DESTINATION ${XDG_APPS_INSTALL_DIR} )
         
INSTALL( FILES standards.ins 
         DESTINATION ${DATA_INSTALL_DIR}/kmidimon )
