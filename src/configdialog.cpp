/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2009 Pedro Lopez-Cabanillas                        *
 *   plcl@users.sourceforge.net                                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA                                                    *
 ***************************************************************************/

#include "configdialog.h"

bool ConfigDialog::showColumn(int colNum)
{
    switch (colNum) {
    case 0:
        return m_showTimeColumn->isChecked();
    case 1:
        return m_showSourceColumn->isChecked();
    case 2:
        return m_showEventTypeColumn->isChecked();
    case 3:
        return m_showChannelColumn->isChecked();
    case 4:
        return m_showData1Column->isChecked();
    case 5:
        return m_showData2Column->isChecked();
    }
    return false;
}

void ConfigDialog::setShowColumn(int colNum, bool newValue)
{
    switch (colNum) {
    case 0:
        m_showTimeColumn->setChecked(newValue);
        break;
    case 1:
        m_showSourceColumn->setChecked(newValue);
        break;
    case 2:
        m_showEventTypeColumn->setChecked(newValue);
        break;
    case 3:
        m_showChannelColumn->setChecked(newValue);
        break;
    case 4:
        m_showData1Column->setChecked(newValue);
        break;
    case 5:
        m_showData2Column->setChecked(newValue);
        break;
    }
}
