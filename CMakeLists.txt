# The name of our project is "KMIDIMON". CMakeLists files in this project can
# refer to the root source directory of the project as ${KMIDIMON_SOURCE_DIR} 
# and to the root binary directory of the project as ${KMIDIMON_BINARY_DIR}
PROJECT(KMIDIMON)

# CMake supports KDE checks since this version number
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.2)

# Show full commands and messages. It generates a lot of console output.
# It is necessary for Eclipse warning/error parser, 
# but it can be enabled anyway with "make VERBOSE=1"
SET(CMAKE_VERBOSE_MAKEFILE OFF)

# Seems that relative paths break some scripts. Don't set it ON
SET(CMAKE_USE_RELATIVE_PATHS OFF)

# Rebuild the object files if the rules have changed, but not the actual source 
# files or headers (e.g. if you changed the some compiler switches)
SET(CMAKE_SKIP_RULE_DEPENDENCY OFF)

# Misc settings
SET(CMAKE_COLOR_MAKEFILE ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

# Additional CMake modules for 3rd party library checks reside here
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_admin")

# KMIDIMON version string
SET (VERSION_MAJOR "0")
SET (VERSION_MINOR "5")
SET (VERSION_PATCH "0")
SET (VERSION_SUFFIX "")
SET (VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}${VERSION_SUFFIX})
ADD_DEFINITIONS(-D'VERSION="${VERSION}"')

# User options
OPTION(WANT_DEBUG   "Include debug support" OFF)
OPTION(WANT_FULLDBG "Full debug support (BIG executables!)" OFF)
OPTION(WANT_PCH     "Use precompiled headers" OFF)

# Check for KDE3
FIND_PACKAGE(KDE3 REQUIRED QUIET)
IF(QT_FOUND)
    ADD_DEFINITIONS(${QT_DEFINITIONS})
    MESSAGE(STATUS "Found Qt3 (version ${qt_version_str})")
    #MESSAGE("QT_LIBRARIES: ${QT_LIBRARIES}")
ELSE(QT_FOUND)
    MESSAGE(FATAL_ERROR "Qt3 package not found")
ENDIF(QT_FOUND)
IF(KDE3_FOUND)
    ADD_DEFINITIONS(${KDE3_DEFINITIONS})
    #MESSAGE("KDE3PREFIX: ${KDE3PREFIX}")
    KDE3_PRINT_RESULTS()
    #MESSAGE("QT_AND_KDECORE_LIBS: ${QT_AND_KDECORE_LIBS}")
ELSE(KDE3_FOUND)    
    MESSAGE(FATAL_ERROR "KDE3 Not found")
ENDIF(KDE3_FOUND)

# Clear the flags set by FindKDE3 
SET(CMAKE_CXX_FLAGS "")

# Provide the KDE3 prefix as a default prefix (if the user has CMake 2.4.4)
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX ${KDE3PREFIX} CACHE PATH
        "Install path prefix, prepended onto install directories." FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT) 

# Check for PKG-CONFIG
FIND_PACKAGE(PkgConfig REQUIRED)
IF(PKG_CONFIG_FOUND)
    MESSAGE(STATUS "Program pkg-config found (${PKG_CONFIG_EXECUTABLE})")
ELSE(PKG_CONFIG_FOUND)
    MESSAGE(FATAL_ERROR "Program pkg-config not found")
ENDIF(PKG_CONFIG_FOUND)

# Initialize some variables
SET(HAVE_ALSA FALSE)

# Check for ALSA 1.0
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
    
FIND_PACKAGE(MSGFMT REQUIRED)
IF(MSGFMT_FOUND)
    MESSAGE(STATUS "Program msgfmt found (${MSGFMT_EXECUTABLE})")
ENDIF(MSGFMT_FOUND)

FIND_PACKAGE(MEINPROC REQUIRED)
IF(MEINPROC_FOUND)
    MESSAGE(STATUS "Program meinproc found (${MEINPROC_EXECUTABLE})")
ENDIF(MEINPROC_FOUND)

# Check GCC for PCH support
SET(USE_PCH FALSE)
IF(WANT_PCH)
    FIND_PACKAGE(PCHSupport)
    IF(PCHSupport_FOUND)
        SET(USE_PCH TRUE)
        MESSAGE(STATUS "Enabling precompiled headers for GCC ${gcc_compiler_version}") 
    ENDIF(PCHSupport_FOUND)
ENDIF(WANT_PCH)

SET(CMAKE_CXX_FLAGS_RELEASE         "-O2 -w -fexceptions -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "-O2 -g -Wall -fexceptions -DDEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG           "-O0 -g3 -Wall -fexceptions -DDEBUG")

MESSAGE(STATUS "kmidimon ${VERSION} will be built for install into ${CMAKE_INSTALL_PREFIX}")

IF(WANT_DEBUG)
    IF(WANT_FULLDBG)
        MESSAGE(STATUS "Configured to compile including full debug information in the executables")
        SET(CMAKE_BUILD_TYPE Debug CACHE STRING
            "Choose the type of build, options are: Debug Release RelWithDebInfo" FORCE)
    ELSE(WANT_FULLDBG)
        MESSAGE(STATUS "Configured to compile including debug information in the executables")
        SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
            "Choose the type of build, options are: Debug Release RelWithDebInfo" FORCE)
    ENDIF(WANT_FULLDBG)
ELSE(WANT_DEBUG)
    MESSAGE(STATUS "Configured to compile for release without debug information")
    SET(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo" FORCE)
ENDIF(WANT_DEBUG)

# Recurse into the "src" subdirectory. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
# Output directory will be "RGbuild/"
ADD_SUBDIRECTORY(src build)

# Build and install the translations directory
ADD_SUBDIRECTORY(po build/po)

# Build and install the documentation directory
ADD_SUBDIRECTORY(doc build/doc)

# uninstall custom target
CONFIGURE_FILE(
    "${CMAKE_SOURCE_DIR}/cmake_admin/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# Custom target trying to clean the CMake generated files
ADD_CUSTOM_TARGET ( distclean
    COMMAND rm -f  {,src/,po/,doc/}CMakeCache.txt
    COMMAND rm -f  {,src/,po/,doc/}cmake_install.cmake
    COMMAND rm -f  {,src/,po/,doc/}cmake_uninstall.cmake
    COMMAND rm -f  {,src/,po/,doc/}CPackConfig.cmake
    COMMAND rm -f  {,src/,po/,doc/}CPackSourceConfig.cmake
    COMMAND rm -f  {,src/,po/,doc/}install_manifest.txt
    COMMAND rm -f  {,src/,po/,doc/}progress.make
    COMMAND rm -f  {,src/,po/,doc/}kmidimon.{fedora,mdk}.spec
    COMMAND rm -f  {,src/,po/,doc/}Makefile
    COMMAND rm -rf {,src/,po/,doc/}CMakeFiles
    COMMAND rm -rf {,src/,po/,doc/}_CPack_Packages
    COMMAND rm -rf build/*
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
ADD_DEPENDENCIES(distclean clean)

# configure the RPM specs
CONFIGURE_FILE(
    "${CMAKE_SOURCE_DIR}/kmidimon.fedora.spec.in"
    "${CMAKE_SOURCE_DIR}/kmidimon.fedora.spec"
    IMMEDIATE @ONLY)
CONFIGURE_FILE(
    "${CMAKE_SOURCE_DIR}/kmidimon.mdk.spec.in"
    "${CMAKE_SOURCE_DIR}/kmidimon.mdk.spec"
    IMMEDIATE @ONLY)

# tarball target
ADD_CUSTOM_TARGET ( tarball
    COMMAND mkdir -p kmidimon-${VERSION}
    COMMAND cp -r cmake_admin kmidimon-${VERSION}
    COMMAND cp -r doc kmidimon-${VERSION}
    COMMAND cp -r po kmidimon-${VERSION}
    COMMAND cp -r src kmidimon-${VERSION}
    COMMAND cp CMakeLists.txt AUTHORS COPYING ChangeLog INSTALL NEWS README TODO *.spec.in kmidimon-${VERSION}
    COMMAND tar -cj --exclude CVS --exclude .cvsignore -f kmidimon-${VERSION}.tar.bz2 kmidimon-${VERSION}
    COMMAND rm -rf kmidimon-${VERSION}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
