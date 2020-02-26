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

install( FILES kmidimon.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

install( FILES kmidimon.html kmidimon1.png kmidimon2.png kmidimon3.png qjackctl.png
    DESTINATION ${CMAKE_INSTALL_DOCDIR} )
