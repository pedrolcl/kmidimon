/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005 by Pedro Lopez-Cabanillas                          *
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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/*
 * Copyright (C) 2005 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
 */

#include <kmainwindow.h>
#include <klocale.h>
#include <kaction.h>
#include <kapplication.h>
#include <kfiledialog.h>
#include <kedittoolbar.h>

#include "kmidimon.h"
#include "kmidimonwidget.h"
#include "sequencerclient.h"
#include "configdialog.h"

KMidimon::KMidimon()
    : KMainWindow( 0, "KMidimon" )
{
    m_widget = new KMidimonWidget( this );
    m_client = new SequencerClient( this );
    setCentralWidget( m_widget );
    setupActions();
    setAutoSaveSettings();
    readConfiguration();
    record();
}

KMidimon::~KMidimon()
{
    if (m_client->running()) {
    	m_client->terminate();
    	m_client->wait();
    }
    delete m_client;
}

void KMidimon::setupActions()
{
    KStdAction::quit( kapp, SLOT(quit()), actionCollection() );
    KStdAction::openNew( this, SLOT(fileNew()), actionCollection() );
    m_save = KStdAction::saveAs( this, SLOT(fileSave()), 
				 actionCollection() );
    m_prefs = KStdAction::preferences( this, SLOT(preferences()), 
				       actionCollection() );
    KStdAction::configureToolbars( this, SLOT(editToolbars()), 
				   actionCollection() );
    m_record = new KAction( i18n("&Record"), "kmidimon_record", 
    			    KShortcut( Key_R ), this, SLOT(record()), 
    			    actionCollection(), "record" );    
    m_stop = new KAction( i18n("&Stop"), "player_stop",  KShortcut( Key_S ),
			  this, SLOT(stop()), actionCollection(), "stop");    
    setStandardToolBarMenuEnabled( true );                 
    createGUI();
}

void KMidimon::customEvent( QCustomEvent * e )
{
    if(e->type() == MONITOR_EVENT_TYPE) {
	m_widget->add((MidiEvent *)e);	
    }
}

void KMidimon::fileNew()
{
    m_widget->clear();
}

void KMidimon::fileSave()
{
    QString path = KFileDialog::getSaveFileName(":MIDIMONITOR",
			i18n("*.txt|Plain text files (*.txt)"),
			this, i18n("Save MIDI monitor data"));
    if (!path.isNull()) {
    	m_widget->saveTo(path);
    }
}

bool KMidimon::queryExit()
{
    saveConfiguration();
    return true;
}

void KMidimon::saveConfiguration()
{
    KConfig *config = kapp->config();
    config->setGroup("Settings");
    config->writeEntry("resolution", m_client->getResolution());
    config->writeEntry("tempo", m_client->getTempo());
    config->writeEntry("tick_time", m_client->isTickTime());
    config->writeEntry("alsa", m_client->isRegAlsaMsg());
    config->writeEntry("channel", m_client->isRegChannelMsg());
    config->writeEntry("common", m_client->isRegCommonMsg());
    config->writeEntry("realtime", m_client->isRegRealTimeMsg());
    config->writeEntry("sysex", m_client->isRegSysexMsg());
}

void KMidimon::readConfiguration()
{
    KConfig *config = kapp->config();
    config->setGroup("Settings");
    m_client->setResolution(config->readNumEntry("resolution", RESOLUTION));
    m_client->setTempo(config->readNumEntry("tempo", TEMPO_BPM));
    m_client->setTickTime(config->readBoolEntry("tick_time", true));
    m_client->setRegAlsaMsg(config->readBoolEntry("alsa", true));
    m_client->setRegChannelMsg(config->readBoolEntry("channel", true));
    m_client->setRegCommonMsg(config->readBoolEntry("common", true));
    m_client->setRegRealTimeMsg(config->readBoolEntry("realtime", true));
    m_client->setRegSysexMsg(config->readBoolEntry("sysex", true));
    m_client->queue_set_tempo();
    m_client->change_port_settings();
}

void KMidimon::preferences()
{
    ConfigDialog dlg;
    dlg.setTempo(m_client->getTempo());
    dlg.setResolution(m_client->getResolution());
    if (m_client->isTickTime()) {
    	dlg.setMusicalTime();
    } else {
    	dlg.setClockTime();
    }
    dlg.setRegAlsaMsg(m_client->isRegAlsaMsg());
    dlg.setRegChannelMsg(m_client->isRegChannelMsg());
    dlg.setRegCommonMsg(m_client->isRegCommonMsg());
    dlg.setRegRealTimeMsg(m_client->isRegRealTimeMsg());
    dlg.setRegSysexMsg(m_client->isRegSysexMsg());
    if (dlg.exec()) {
    	m_client->setTempo(dlg.getTempo());
    	m_client->setResolution(dlg.getResolution());
	m_client->setTickTime(dlg.isMusicalTime());
	m_client->setRegAlsaMsg(dlg.isRegAlsaMsg());
	m_client->setRegChannelMsg(dlg.isRegChannelMsg());
	m_client->setRegCommonMsg(dlg.isRegCommonMsg());
	m_client->setRegRealTimeMsg(dlg.isRegRealTimeMsg());
	m_client->setRegSysexMsg(dlg.isRegSysexMsg());
    	m_client->queue_set_tempo();
	m_client->change_port_settings();
    }
}

void KMidimon::record()
{
    if (!m_client->queue_running()) {
	m_client->queue_start();
    }
    updateState();
}

void KMidimon::stop()
{
    if (m_client->queue_running()) {
	m_client->queue_stop();
    }
    updateState();
}

void KMidimon::updateState() 
{
    QString state( m_client->queue_running() ? "recording" : "stopped" );
    setCaption( QString("ALSA MIDI Monitor [%1]").arg(state) );
    slotStateChanged( state );
}

void KMidimon::editToolbars()
{
    KEditToolbar dlg(actionCollection());
    if (dlg.exec()) {
	createGUI();
    }
}

#include "kmidimon.moc"
