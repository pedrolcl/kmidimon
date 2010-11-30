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

kde4_create_handbook( index.docbook 
                      INSTALL_DESTINATION ${HTML_INSTALL_DIR}/ja 
                      SUBDIR kmidimon )

kde4_create_manpage( man-kmidimon.1.docbook 1 
                     INSTALL_DESTINATION ${MAN_INSTALL_DIR}/ja )
