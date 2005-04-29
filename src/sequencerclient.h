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
#ifndef SEQUENCERCLIENT_H
#define SEQUENCERCLIENT_H

#include <qthread.h>
#include <qevent.h>
#include <klocale.h>
#include <alsa/asoundlib.h>

#define MONITOR_EVENT_TYPE (QEvent::User + 1)
#define TEMPO_BPM 120
#define RESOLUTION 240

class MidiEvent : public QCustomEvent
{
public:
     MidiEvent( QString time, 
     		QString kind, 
     		QString ch = NULL, 
     		QString d1 = NULL, 
     		QString d2 = NULL)
	: QCustomEvent( MONITOR_EVENT_TYPE ), 
	m_time(time), 
	m_kind(kind),
	m_chan(ch),
	m_data1(d1),
	m_data2(d2) {}
	
	QString getTime() { return m_time; }
	QString getKind() { return m_kind; }
	QString getChannel() { return m_chan; }
	QString getData1() { return m_data1; }
	QString getData2() { return m_data2; }
private:
     QString m_time;
     QString m_kind;
     QString m_chan;
     QString m_data1;
     QString m_data2;
};

/**
@author Pedro Lopez-Cabanillas
*/
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
    
    void setTempo(int newValue) { m_tempo = newValue; }
    void setResolution(int newValue) { m_resolution = newValue; }
    void setTickTime(bool newValue) { m_tickTime = newValue; }
    void setRegChannelMsg(bool newValue) { m_channel = newValue; }
    void setRegCommonMsg(bool newValue) { m_common = newValue; }
    void setRegRealTimeMsg(bool newValue) { m_realtime = newValue; }
    void setRegSysexMsg(bool newValue) { m_sysex = newValue; }
    void setRegAlsaMsg(bool newValue) { m_alsa = newValue; }
    
private:
    int checkAlsaError(int rc, const char *message);
    MidiEvent *build_midi_event(snd_seq_event_t *ev);
    MidiEvent *build_sysex_event(snd_seq_event_t *ev);
    MidiEvent *build_note_event( snd_seq_event_t *ev, QString statusText );
    MidiEvent *build_control_event( snd_seq_event_t *ev, QString statusText );
    MidiEvent *build_controlv_event( snd_seq_event_t *ev, QString statusText );
    QString event_time(snd_seq_event_t *ev);
    QString event_addr(snd_seq_event_t *ev);
    QString event_sender(snd_seq_event_t *ev);
    QString event_dest(snd_seq_event_t *ev);
    
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

    snd_seq_t *m_handle;
    int m_client;
    int m_input;
    int m_queue;
    
};

#endif
