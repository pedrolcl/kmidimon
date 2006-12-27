/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2006 Pedro Lopez-Cabanillas                        *
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

#ifndef SEQUENCERCLIENT_H
#define SEQUENCERCLIENT_H

#include <qthread.h>
#include <qevent.h>
#include <qmap.h>
#include <klocale.h>
#include <alsa/asoundlib.h>

#define MONITOR_EVENT_TYPE (QEvent::User + 1)
#define TEMPO_BPM 120
#define RESOLUTION 240

class MidiEvent : public QCustomEvent
{
public:
     MidiEvent( QString time, 
     		QString src,
     		QString kind, 
     		QString ch = QString::null, 
     		QString d1 = QString::null, 
     		QString d2 = QString::null)
	: QCustomEvent( MONITOR_EVENT_TYPE ), 
	m_time(time), 
	m_src(src),
	m_kind(kind),
	m_chan(ch),
	m_data1(d1),
	m_data2(d2) {}
	
	QString getTime() { return m_time; }
	QString getSource() { return m_src; }
	QString getKind() { return m_kind; }
	QString getChannel() { return m_chan; }
	QString getData1() { return m_data1; }
	QString getData2() { return m_data2; }
private:
     QString m_time;
     QString m_src;
     QString m_kind;
     QString m_chan;
     QString m_data1;
     QString m_data2;
};

class SequencerClient: public QThread
{
public:
    SequencerClient(QWidget *parent);
    ~SequencerClient();
    virtual void run();
    bool queue_running() { return m_queue_running; }
    void queue_start();
    void queue_stop();
    void queue_set_tempo();
    void change_port_settings();
    
    int getTempo() { return m_tempo; }
    int getResolution() { return m_resolution; }
    bool isTickTime() { return m_tickTime; }
    bool isRegChannelMsg() { return m_channel; }
    bool isRegCommonMsg() { return m_common; }
    bool isRegRealTimeMsg() { return m_realtime; }
    bool isRegSysexMsg() { return m_sysex; }
    bool isRegAlsaMsg() { return m_alsa; }
    bool showClientNames() { return m_showClientNames; }
    bool translateSysex() { return m_translateSysex; }
    
    void setTempo(int newValue) { m_tempo = newValue; }
    void setResolution(int newValue) { m_resolution = newValue; }
    void setTickTime(bool newValue) { m_tickTime = newValue; }
    void setRegChannelMsg(bool newValue) { m_channel = newValue; }
    void setRegCommonMsg(bool newValue) { m_common = newValue; }
    void setRegRealTimeMsg(bool newValue) { m_realtime = newValue; }
    void setRegSysexMsg(bool newValue) { m_sysex = newValue; }
    void setRegAlsaMsg(bool newValue) { m_alsa = newValue; }
    void setShowClientNames(bool newValue) { m_showClientNames = newValue; }
    void setTranslateSysex(bool newValue) { m_translateSysex = newValue; }

    void connect_port(QString name);
    void disconnect_port(QString name);
    void connect_all();
    void disconnect_all();
    QStringList inputConnections();
    QStringList outputConnections();
    QStringList list_subscribers();
    
private:
    int checkAlsaError(int rc, const char *message);
    MidiEvent *build_midi_event(snd_seq_event_t *ev);
    MidiEvent *build_translated_sysex(snd_seq_event_t *ev);
    MidiEvent *build_sysex_event(snd_seq_event_t *ev);
    MidiEvent *build_note_event( snd_seq_event_t *ev, QString statusText );
    MidiEvent *build_control_event( snd_seq_event_t *ev, QString statusText );
    MidiEvent *build_controlv_event( snd_seq_event_t *ev, QString statusText );
    MidiEvent *build_common_event( snd_seq_event_t *ev, QString statusText,
    				   QString param = NULL );
    MidiEvent *build_realtime_event( snd_seq_event_t *ev, QString statusText );
    MidiEvent *build_alsa_event( snd_seq_event_t *ev, QString statusText,
				 QString srcAddr = NULL, QString dstAddr = NULL);
				 
    QString client_name(int client_number);
    QString event_source(snd_seq_event_t *ev);
    QString event_time(snd_seq_event_t *ev);
    QString event_client(snd_seq_event_t *ev);
    QString event_addr(snd_seq_event_t *ev);
    QString event_sender(snd_seq_event_t *ev);
    QString event_dest(snd_seq_event_t *ev);
    QString common_param(snd_seq_event_t *ev);
    QStringList list_ports(unsigned int mask);
    void refreshClientList();
    
    QWidget *m_widget;
    bool m_queue_running;
    int m_resolution;
    int m_tempo;
    bool m_tickTime;
    bool m_channel;
    bool m_common;
    bool m_realtime;
    bool m_sysex;
    bool m_alsa;
    bool m_showClientNames;
    bool m_translateSysex;
    bool m_needsRefresh;

    snd_seq_t *m_handle;
    int m_client;
    int m_input;
    int m_queue;
    QMap<int, QString> m_clients;
};

#endif
