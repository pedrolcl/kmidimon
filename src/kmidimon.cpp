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

#include <qpopupmenu.h>
#include <kmainwindow.h>
#include <klocale.h>
#include <kaction.h>
#include <kapplication.h>
#include <kfiledialog.h>
#include <kedittoolbar.h>
#include <klistview.h>

#include "kmidimon.h"
#include "kmidimonwidget.h"
#include "sequencerclient.h"
#include "configdialog.h"
#include "connectdlg.h"
#include "debugdef.h"

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
			  
    m_connectAll = new KAction( i18n("&Connect All"), KShortcut::null(),
				this, SLOT(connectAll()), 
				actionCollection(), "connect_all" );    
				
    m_disconnectAll = new KAction( i18n("&Disconnect All"), KShortcut::null(),
				   this, SLOT(disconnectAll()), 
				   actionCollection(), "disconnect_all" );
    
    m_configConns = new KAction( i18n("Con&figure Connections"), KShortcut::null(),
				 this, SLOT(configConnections()), 
				 actionCollection(), "connections_dialog" );

    m_popupAction[0] = new KToggleAction( i18n("&Time"), KShortcut::null(),
                           this, SLOT(toggleColumn0()), actionCollection(), "show_time" );
    m_popupAction[1] = new KToggleAction( i18n("&Source"), KShortcut::null(),
                           this, SLOT(toggleColumn1()), actionCollection(), "show_source" );
    m_popupAction[2] = new KToggleAction( i18n("&Event Kind"), KShortcut::null(),
                           this, SLOT(toggleColumn2()), actionCollection(), "show_kind" );
    m_popupAction[3] = new KToggleAction( i18n("&Channel"), KShortcut::null(),
                           this, SLOT(toggleColumn3()), actionCollection(), "show_channel" );
    m_popupAction[4] = new KToggleAction( i18n("Data &1"), KShortcut::null(),
                           this, SLOT(toggleColumn4()), actionCollection(), "show_data1" );
    m_popupAction[5] = new KToggleAction( i18n("Data &2"), KShortcut::null(),
                           this, SLOT(toggleColumn5()), actionCollection(), "show_data2" );
			  
    setStandardToolBarMenuEnabled( true );                 
    createGUI();
    
    popup = static_cast<QPopupMenu *>(factory()->container("popup", this)); 
    Q_CHECK_PTR( popup );
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
    int i;
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
    config->writeEntry("client_names", m_client->showClientNames());
    config->writeEntry("translate_sysex", m_client->translateSysex());
    config->writeEntry("fixed_font", m_widget->getFixedFont());
    for(i = 0; i < 6; ++i) {
    	config->writeEntry(QString("show_column_%1").arg(i), m_popupAction[i]->isChecked() );
    }
}

void KMidimon::readConfiguration()
{
    int i;
    bool status;
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
    m_client->setShowClientNames(config->readBoolEntry("client_names", false));
    m_client->setTranslateSysex(config->readBoolEntry("translate_sysex", false));
    m_client->queue_set_tempo();
    m_client->change_port_settings();
    m_widget->setFixedFont(config->readBoolEntry("fixed_font", false));
    for(i = 0; i < 6; ++i) {
        status = config->readBoolEntry(QString("show_column_%1").arg(i), true);
        setColumnStatus(i, status);
    }
}

void KMidimon::preferences()
{
	int i;
    bool was_running;
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
    dlg.setShowClientNames(m_client->showClientNames());
    dlg.setTranslateSysex(m_client->translateSysex());
    dlg.setUseFixedFont(m_widget->getFixedFont());
    for(i = 0; i < 6; ++i) {
    	dlg.setShowColumn(i, m_popupAction[i]->isChecked());
    }
    if (dlg.exec()) {
    	was_running = m_client->queue_running();
    	if (was_running) stop();
    	m_client->setTempo(dlg.getTempo());
    	m_client->setResolution(dlg.getResolution());
		m_client->setTickTime(dlg.isMusicalTime());
		m_client->setRegAlsaMsg(dlg.isRegAlsaMsg());
		m_client->setRegChannelMsg(dlg.isRegChannelMsg());
		m_client->setRegCommonMsg(dlg.isRegCommonMsg());
		m_client->setRegRealTimeMsg(dlg.isRegRealTimeMsg());
		m_client->setRegSysexMsg(dlg.isRegSysexMsg());
		m_client->setShowClientNames(dlg.showClientNames());
		m_client->setTranslateSysex(dlg.translateSysex());
    	m_client->queue_set_tempo();
		m_client->change_port_settings();
		m_widget->setFixedFont(dlg.useFixedFont());
		for(i = 0; i < 6; ++i) {
            setColumnStatus(i, dlg.showColumn(i));
		}
		if (was_running) record();
    }
}

void KMidimon::record()
{
    if (!m_client->queue_running()) {
		m_client->queue_start();
    }
    updateState();
    slotStateChanged("recording_state");
}

void KMidimon::stop()
{
    if (m_client->queue_running()) {
		m_client->queue_stop();
    }
    updateState();
    slotStateChanged("stopped_state");
}

void KMidimon::updateState() 
{
    QString state( m_client->queue_running() ? i18n("recording") : i18n("stopped") );
    setCaption( i18n("ALSA MIDI Monitor [%1]").arg(state) );
}

void KMidimon::editToolbars()
{
    KEditToolbar dlg(actionCollection());
    if (dlg.exec()) {
		createGUI();
    }
}

void KMidimon::connectAll()
{
    m_client->connect_all();	
}

void KMidimon::disconnectAll()
{
    m_client->disconnect_all();
}

void KMidimon::configConnections()
{
    ConnectDlg dlg( this, m_client->inputConnections(), 
                    m_client->list_subscribers() );
    if (dlg.exec()) {
    	QStringList desired = dlg.getSelected();
	QStringList subs = m_client->list_subscribers();    	
	QStringList::Iterator i;
	for ( i = subs.begin(); i != subs.end(); ++i) {
	    if (desired.contains(*i) == 0) {
                //DEBUGSTREAM << "desired.contains(" << *i << ")" << endl;
	    	m_client->disconnect_port(*i);
	    }
	}
	for ( i = desired.begin(); i != desired.end(); ++i) {
	    if (subs.contains(*i) == 0) {
                //DEBUGSTREAM << "subs.contains(" << *i << ")" << endl;
		m_client->connect_port(*i);
	    }
	}
    }
}

void KMidimon::setColumnStatus(int colNum, bool status)
{
    m_widget->setShowColumn(colNum, status);
    m_popupAction[colNum]->setChecked(status);
}

void KMidimon::toggleColumn(int colNum)
{
    m_widget->setShowColumn(colNum, m_popupAction[colNum]->isChecked());
}

void KMidimon::toggleColumn0()
{
    toggleColumn(0);
}

void KMidimon::toggleColumn1()
{
    toggleColumn(1);
}

void KMidimon::toggleColumn2()
{
    toggleColumn(2);
}

void KMidimon::toggleColumn3()
{
    toggleColumn(3);
}

void KMidimon::toggleColumn4()
{
    toggleColumn(4);
}

void KMidimon::toggleColumn5()
{
    toggleColumn(5);
}

void KMidimon::contextMenuEvent( QContextMenuEvent *ev )
{
    Q_CHECK_PTR( popup );
    popup->popup( ev->pos() );
}

#include "kmidimon.moc"
