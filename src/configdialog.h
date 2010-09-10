/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2010 Pedro Lopez-Cabanillas                        *
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

#include <KDE/KDialog>
#include "ui_configdialogbase.h"

class ConfigDialog : public KDialog
{
    Q_OBJECT
public:
    ConfigDialog(QWidget *parent = 0);
    virtual ~ConfigDialog() {}

    int getTempo() { return ui.m_tempo->value(); }
    void setTempo(int newValue) { ui.m_tempo->setValue(newValue); }

    int getResolution() { return ui.m_resolution->value(); }
    void setResolution(int newValue) { ui.m_resolution->setValue(newValue); }

    bool isRegChannelMsg() { return ui.m_channel->isChecked(); }
    bool isRegCommonMsg() { return ui.m_common->isChecked(); }
    bool isRegRealTimeMsg() { return ui.m_realtime->isChecked(); }
    bool isRegSysexMsg() { return ui.m_sysex->isChecked(); }
    bool isRegAlsaMsg() { return ui.m_alsa->isChecked(); }
    bool isRegSmfMsg() { return ui.m_smfmsg->isChecked(); }

    void setRegChannelMsg(bool newValue) { ui.m_channel->setChecked(newValue); }
    void setRegCommonMsg(bool newValue) { ui.m_common->setChecked(newValue); }
    void setRegRealTimeMsg(bool newValue) { ui.m_realtime->setChecked(newValue); }
    void setRegSysexMsg(bool newValue) { ui.m_sysex->setChecked(newValue); }
    void setRegAlsaMsg(bool newValue) { ui.m_alsa->setChecked(newValue); }
    void setRegSmfMsg(bool newValue) { ui.m_smfmsg->setChecked(newValue); }

    bool showClientNames() { return ui.m_showClientNames->isChecked(); }
    void setShowClientNames(bool newValue) { ui.m_showClientNames->setChecked(newValue); }

    bool translateSysex() { return ui.m_translateSysex->isChecked(); }
    void setTranslateSysex(bool newValue) { ui.m_translateSysex->setChecked(newValue); }

    bool useFixedFont() { return ui.m_useFixedFont->isChecked(); }
    void setUseFixedFont(bool newValue) { ui.m_useFixedFont->setChecked(newValue); }

    bool showColumn(int colNum);
    void setShowColumn(int colNum, bool newValue);

    bool translateNotes() { return ui.m_translateNotes->isChecked(); }
    void setTranslateNotes(bool newValue) { ui.m_translateNotes->setChecked(newValue); }

    bool translateCtrls() { return ui.m_translateCtrls->isChecked(); }
    void setTranslateCtrls(bool newValue) { ui.m_translateCtrls->setChecked(newValue); }

    QString getInstrumentName() { return ui.m_instruments->currentText(); }
    void setInstrumentName(const QString& name);
    void setInstruments(const QStringList& items);

    void initEncodings();
    QString getEncoding();
    void setEncoding(const QString& name);

    bool requestRealtime() { return ui.m_requestRealtime->isChecked(); }
    void setRequestRealtime(bool newValue) { ui.m_requestRealtime->setChecked(newValue); }

    bool resizeColumns() { return ui.m_resizeColumns->isChecked(); }
    void setResizeColumns(bool newValue) { ui.m_resizeColumns->setChecked(newValue); }

private:
    Ui::ConfigDialogBase ui;
};

#endif
