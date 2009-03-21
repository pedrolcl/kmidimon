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

    int getTempo() { return m_tempo; }
    int getResolution() { return m_resolution; }
    void setTempo(int newValue) { m_tempo = newValue; }
    void setResolution(int newValue) { m_resolution = newValue; }
    void setModel(SequenceModel* m) { m_model = m; }

    void connect_port(QString name);
    void disconnect_port(QString name);
    void connect_all();
    void disconnect_all();
    QStringList inputConnections();
    QStringList list_subscribers();
    void updateModelClients();

public slots:
    /* handler for the sequencer events */
    void sequencerEvent( SequencerEvent* ev );

private:
    QStringList list_ports(PortInfoList& refs);

    bool m_queue_running;
    int m_resolution;
    int m_tempo;
    int m_inputPort;
    int m_queueId;
    int m_clientId;
    MidiClient* m_client;
    MidiQueue* m_queue;
    MidiPort* m_port;
    SequenceModel* m_model;
};

#endif
