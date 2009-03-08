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

#ifndef SEQUENCERCLIENT_H
#define SEQUENCERCLIENT_H

#include <QEvent>
#include <QMap>

#include <queue.h>
#include <port.h>
#include <event.h>

class SequenceItem;
class SequenceModel;

using namespace ALSA::Sequencer;

const int TEMPO_BPM(120);
const int RESOLUTION(240);

class SequencerAdaptor: public QObject
{
	Q_OBJECT
public:
    SequencerAdaptor(QObject *parent);
    ~SequencerAdaptor();
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
    void setModel(SequenceModel* m) { m_model = m; }

    void connect_port(QString name);
    void disconnect_port(QString name);
    void connect_all();
    void disconnect_all();
    QStringList inputConnections();
    QStringList list_subscribers();

public slots:
    /* handler for the sequencer events */
    void sequencerEvent( SequencerEvent* ev );

private:
    SequenceItem *build_midi_event(SequencerEvent *ev);
    SequenceItem *build_translated_sysex(SysExEvent *ev);
    SequenceItem *build_sysex_event(SysExEvent *ev);
    SequenceItem *build_note_event(KeyEvent *ev, QString statusText);
    SequenceItem *build_control_event(ControllerEvent *ev, QString statusText);
    SequenceItem *build_controlv_event(SequencerEvent *ev, QString statusText);
    SequenceItem *build_common_event(SequencerEvent *ev, QString statusText,
                                     QString param = NULL);
    SequenceItem *build_realtime_event(SequencerEvent *ev, QString statusText);
    SequenceItem *build_alsa_event(SequencerEvent *ev, QString statusText,
                                   QString srcAddr = NULL, QString dstAddr = NULL);

    QString client_name(int client_number);
    QString event_source(SequencerEvent *ev);
    QString event_time(SequencerEvent *ev);
    QString event_client(SequencerEvent *ev);
    QString event_addr(SequencerEvent *ev);
    QString event_sender(SequencerEvent *ev);
    QString event_dest(SequencerEvent *ev);
    QString common_param(SequencerEvent *ev);
    QStringList list_ports(PortInfoList& refs);
    void refreshClientList();

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

    int m_inputPort;
    int m_queueId;
    int m_clientId;
    MidiClient* m_client;
    MidiQueue* m_queue;
    MidiPort* m_port;
    SequenceModel* m_model;
};

#endif
