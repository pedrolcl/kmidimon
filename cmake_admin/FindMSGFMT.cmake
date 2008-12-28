# KMetronome - ALSA Sequencer based MIDI metronome
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

#
# Find the msgfmt program
#
# Defined variables:
#  MSGFMT_FOUND
#  MSGFMT_EXECUTABLE
#
# Macro:
#  ADD_TRANSLATIONS
#

IF(MSGFMT_EXECUTABLE)
    SET(MSGFMT_FOUND TRUE)
ELSE(MSGFMT_EXECUTABLE)
    FIND_PROGRAM(MSGFMT_EXECUTABLE
	NAMES msgfmt gmsgfmt
	PATHS /bin /usr/bin /usr/local/bin )
    IF(MSGFMT_EXECUTABLE)
        SET(MSGFMT_FOUND TRUE)
    ELSE(MSGFMT_EXECUTABLE)
	IF(NOT MSGFMT_FIND_QUIETLY)
	    IF(MSGFMT_FIND_REQUIRED)
                MESSAGE(FATAL_ERROR "msgfmt program couldn't be found")
	    ENDIF(MSGFMT_FIND_REQUIRED)
	ENDIF(NOT MSGFMT_FIND_QUIETLY)
    ENDIF(MSGFMT_EXECUTABLE)
    MARK_AS_ADVANCED(MSGFMT_EXECUTABLE)
ENDIF (MSGFMT_EXECUTABLE)

MACRO(ADD_TRANSLATIONS _baseName)
    SET(_outputs)
    FOREACH(_file ${ARGN})
	GET_FILENAME_COMPONENT(_file_we ${_file} NAME_WE)
	SET(_out "${CMAKE_CURRENT_BINARY_DIR}/${_file_we}.gmo")
	SET(_in  "${CMAKE_CURRENT_SOURCE_DIR}/${_file_we}.po")
	ADD_CUSTOM_COMMAND(
	    OUTPUT ${_out}
	    COMMAND ${MSGFMT_EXECUTABLE} -o ${_out} ${_in}
	    DEPENDS ${_in} )
	INSTALL(FILES ${_out}
	    DESTINATION ${LOCALE_INSTALL_DIR}/${_file_we}/LC_MESSAGES/
	    RENAME ${_baseName}.mo )
	SET(_outputs ${_outputs} ${_out})
    ENDFOREACH(_file)
    ADD_CUSTOM_TARGET(translations ALL DEPENDS ${_outputs})
ENDMACRO(ADD_TRANSLATIONS)
