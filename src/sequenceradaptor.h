/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2020 Pedro Lopez-Cabanillas                        *
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
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.*
 ***************************************************************************/

#ifndef SEQUENCERADAPTOR_H
#define SEQUENCERADAPTOR_H

#include <QEvent>
#include <QMap>

#include <drumstick/alsaqueue.h>
#include <drumstick/alsaport.h>
#include <drumstick/alsaevent.h>

class SequenceModel;
class Player;

const int TEMPO_BPM(120);
const int RESOLUTION(240);

class SequencerAdaptor: public QObject
{
	Q_OBJECT
public:
	enum State
	{
	    StoppedState,
	    PlayingState,
	    RecordingState,
	    PausedState,
	    ErrorState
	};

    SequencerAdaptor(QObject *parent);
    ~SequencerAdaptor();

    bool isRecording() { return m_state == RecordingState; }
    bool isPaused() { return m_state == PausedState; }
    bool isPlaying();
    State currentState() { return m_state; }

    void queue_set_tempo();

    void record();
    void play();
    void pause(bool checked);
    void stop();
    void rewind();
    void forward();

    int getTempo() { return m_tempo; }
    int getResolution() { return m_resolution; }
    void setTempo(int newValue) { m_tempo = newValue; }
    void setResolution(int newValue) { m_resolution = newValue; }
    void setModel(SequenceModel* m);

    void connect_input(QString name);
    void disconnect_input(QString name);
    void connect_output(QString name);
    void disconnect_output(QString name);
    void connect_all_inputs();
    void disconnect_all_inputs();
    QStringList inputConnections();
    QStringList list_subscribers();
    QStringList outputConnections();
    QString output_subscriber();
    void updateModelClients();
    void setPosition(const int pos);
    void setTempoFactor(double factor);
    void removeTrackEvents(int track);
    void setRequestRealtime(bool newValue);
    bool requestedRealtime();

public slots:
    /* handler for the sequencer events */
    void sequencerEvent( drumstick::ALSA::SequencerEvent* ev );
    void songFinished();
    void shutupSound();
    void setLoop(bool enable);

signals:
    void signalTicks(int tick);
    void finished();

private:
    QStringList list_ports(drumstick::ALSA::PortInfoList& refs);

    State m_state;
    int m_resolution;
    int m_tempo;

    drumstick::ALSA::MidiClient* m_client;
    drumstick::ALSA::MidiQueue* m_queue;
    drumstick::ALSA::MidiPort* m_port;
    SequenceModel* m_model;
    Player* m_player;
};

#endif
