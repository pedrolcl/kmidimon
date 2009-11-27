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

#ifndef SEQUENCERADAPTOR_H
#define SEQUENCERADAPTOR_H

#include <QEvent>
#include <QMap>

#include <alsaqueue.h>
#include <alsaport.h>
#include <alsaevent.h>

class SequenceModel;
class Player;

using namespace aseqmm;

const int TEMPO_BPM(120);
const int RESOLUTION(240);

class SequencerAdaptor: public QObject
{
	Q_OBJECT
public:
    SequencerAdaptor(QObject *parent);
    ~SequencerAdaptor();

    bool isRecording() { return m_recording; }
    bool isPlaying();

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

public slots:
    /* handler for the sequencer events */
    void sequencerEvent( SequencerEvent* ev );
    void songFinished();
    void shutupSound();

signals:
    void signalTicks(int tick);

private:
    QStringList list_ports(PortInfoList& refs);

    bool m_recording;
    int m_resolution;
    int m_tempo;

    MidiClient* m_client;
    MidiQueue* m_queue;
    MidiPort* m_port;
    SequenceModel* m_model;
    Player* m_player;
};

#endif
