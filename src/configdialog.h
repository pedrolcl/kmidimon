/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2008 Pedro Lopez-Cabanillas                        *
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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <qcheckbox.h>
#include <qradiobutton.h>
#include <knuminput.h>

#include "configdialogbase.h"

/**
@author Pedro Lopez-Cabanillas
*/
class ConfigDialog : public ConfigDialogBase
{
    Q_OBJECT
    
public:
    ConfigDialog():ConfigDialogBase() {}
    
    int getTempo() { return m_tempo->value(); }
    int getResolution() { return m_resolution->value(); }
    bool isMusicalTime() { return m_musicalTime->isChecked(); }
    bool isClockTime() { return m_clockTime->isChecked(); }
    
    bool isRegChannelMsg() { return m_channel->isChecked(); }
    bool isRegCommonMsg() { return m_common->isChecked(); }
    bool isRegRealTimeMsg() { return m_realtime->isChecked(); }
    bool isRegSysexMsg() { return m_sysex->isChecked(); }
    bool isRegAlsaMsg() { return m_alsa->isChecked(); }
    
    void setTempo(int newValue) { m_tempo->setValue(newValue); }
    void setResolution(int newValue) { m_resolution->setValue(newValue); }
    void setMusicalTime() { m_musicalTime->setChecked(true); }
    void setClockTime() { m_clockTime->setChecked(true); }
    
    void setRegChannelMsg(bool newValue) { m_channel->setChecked(newValue); }
    void setRegCommonMsg(bool newValue) { m_common->setChecked(newValue); }
    void setRegRealTimeMsg(bool newValue) { m_realtime->setChecked(newValue); }
    void setRegSysexMsg(bool newValue) { m_sysex->setChecked(newValue); }
    void setRegAlsaMsg(bool newValue) { m_alsa->setChecked(newValue); }
    
    bool showClientNames() { return m_showClientNames->isChecked(); }
    void setShowClientNames(bool newValue) { m_showClientNames->setChecked(newValue); }
    
    bool translateSysex() { return m_translateSysex->isChecked(); }
    void setTranslateSysex(bool newValue) { m_translateSysex->setChecked(newValue); }
    
    bool useFixedFont() { return m_useFixedFont->isChecked(); }
    void setUseFixedFont(bool newValue) { m_useFixedFont->setChecked(newValue); }
    
    bool showColumn(int colNum);
    void setShowColumn(int colNum, bool newValue);
};

#endif
