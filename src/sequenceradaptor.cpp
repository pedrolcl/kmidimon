/***************************************************************************
 *   Drumstick MIDI monitor based on the ALSA Sequencer                    *
 *   Copyright (C) 2005-2024 Pedro Lopez-Cabanillas                        *
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

#include <QApplication>
#include <QStringList>
#include <drumstick/alsaclient.h>
#include <drumstick/alsaport.h>
#include <drumstick/alsaqueue.h>
#include <drumstick/alsaevent.h>
#include <drumstick/subscription.h>

#include "sequenceradaptor.h"
#include "sequenceitem.h"
#include "sequencemodel.h"
#include "player.h"

using namespace std;
using namespace drumstick::ALSA;

SequencerAdaptor::SequencerAdaptor(QObject *parent):
    QObject(parent),
    m_state(ErrorState),
    m_resolution(RESOLUTION),
    m_tempo(TEMPO_BPM),
    m_thru(false)
{
    qRegisterMetaType<SequencerEvent*>();

    m_client = new MidiClient(this);
    m_client->open();
    m_client->setPoolOutput(50); // small buffer size, for better feedback
    m_client->setClientName("KMidimon");
    connect(m_client,
            &MidiClient::eventReceived,
            this,
            &SequencerAdaptor::sequencerEvent,
            Qt::UniqueConnection);

    m_queue = m_client->createQueue("KMidimon");

    m_port = new MidiPort(this);
    m_port->attach( m_client );
    m_port->setPortName("KMidimon");
    m_port->setCapability( SND_SEQ_PORT_CAP_READ |
                           SND_SEQ_PORT_CAP_WRITE |
                           SND_SEQ_PORT_CAP_SUBS_READ |
                           SND_SEQ_PORT_CAP_SUBS_WRITE );
    m_port->setPortType( SND_SEQ_PORT_TYPE_MIDI_GENERIC |
                         SND_SEQ_PORT_TYPE_APPLICATION );
    m_port->setMidiChannels(16);
    m_port->setTimestamping(true);
    m_port->setTimestampReal(false);
    m_port->setTimestampQueue(m_queue->getId());
    m_port->subscribeFromAnnounce();

    m_player = new Player(m_client, m_port->getPortId());
    connect(m_player, &Player::playbackStopped, this, &SequencerAdaptor::shutupSound);
    connect(m_player, &Player::playbackFinished, this, &SequencerAdaptor::songFinished);
    m_client->setRealTimeInput(false);
    m_client->startSequencerInput();
    m_state = StoppedState;
}

SequencerAdaptor::~SequencerAdaptor()
{
    m_client->stopSequencerInput();
    m_port->detach();
    m_client->close();
    delete m_player;
}

void SequencerAdaptor::setModel(SequenceModel* m)
{
    m_model = m;
    m_model->updateQueue(m_queue->getId());
    m_model->updatePort(m_port->getPortId());
}

void SequencerAdaptor::updateModelClients()
{
    ClientsMap m;
    ClientInfoList list = m_client->getAvailableClients();
    ClientInfoList::ConstIterator it;
    for(it = list.constBegin(); it != list.constEnd(); ++it) {
        ClientInfo c = *it;
        m.insert(c.getClientId(), c.getName());
    }
    m_model->updateClients(m);
}

void SequencerAdaptor::sequencerEvent(SequencerEvent* ev)
{
    switch(m_state) {
    case RecordingState: {
        QueueStatus s = m_queue->getStatus();
        unsigned int ticks = s.getTickTime();
        double seconds = s.getClockTime();
        ev->setSubscribers();
        ev->scheduleTick(m_queue->getId(), ev->getTick(), false);
        SequenceItem itm(seconds, ticks, m_model->currentTrack(), ev);
        if (SequencerEvent::isClient(ev)) {
            updateModelClients();
        }
        m_model->addItem(itm);
        if (m_thru && !SequencerEvent::isConnectionChange(ev)) {
            m_client->outputDirect(ev);
        }
        break;
    }
    default:
        if ((ev != nullptr) && (ev->getSequencerType() == SND_SEQ_EVENT_ECHO)) {
            EchoEvent *echo = static_cast<EchoEvent*>(ev);
            emit signalTicks(echo->getValue());
        }
        delete ev;
    }
}

void SequencerAdaptor::play()
{
    if (!m_model->isEmpty() && !m_player->isRunning()) {
        if (m_player->getInitialPosition() == 0) {
            if (m_tempo == 0) {
                return;
            }
            m_player->setSong(m_model->getSong(), m_resolution);
            queue_set_tempo();
            m_client->drainOutput();
        }
        m_player->start();
        m_state = PlayingState;
    }
}

void SequencerAdaptor::pause(bool checked)
{
    if (checked) {
        if (m_player->isRunning()) {
            m_player->stop();
            m_player->setPosition(m_queue->getStatus().getTickTime());
            m_state = PausedState;
        }
    } else {
        m_player->start();
        m_state = PlayingState;
    }
}

void SequencerAdaptor::stop()
{
    if (m_state == RecordingState ||
        m_state == PausedState ||
        m_state == PlayingState ||
        m_player->isRunning()) {
        m_queue->stop();
        m_queue->clear();
        m_player->stop();
        m_state = StoppedState;
        songFinished();
    }
}

void SequencerAdaptor::rewind()
{
    if (m_player != nullptr) m_player->resetPosition();
    m_model->setCurrentRow(0);
    m_queue->setTickPosition(0);
}

void SequencerAdaptor::forward()
{
    int r = m_model->rowCount(QModelIndex()) - 1;
    setPosition(r);
}

void SequencerAdaptor::record()
{
    if (m_state != RecordingState) {
        QueueStatus s = m_queue->getStatus();
        if (s.getTickTime() == 0) m_queue->start();
        else m_queue->continueRunning();
        s = m_queue->getStatus();
        if (s.isRunning())
            m_state = RecordingState;
    }
}

void SequencerAdaptor::setPosition(const int pos)
{
    const SequencerEvent* ev = m_model->getEvent(pos);
    if (ev != nullptr) {
        int t = ev->getTick();
        if (m_player != nullptr) m_player->setPosition(t);
        m_model->setCurrentRow(pos);
        m_queue->setTickPosition(t);
    }
}

void SequencerAdaptor::queue_set_tempo()
{
    QueueTempo tempo = m_queue->getTempo();
    tempo.setPPQ(m_resolution);
    tempo.setNominalBPM(m_tempo);
    m_queue->setTempo(tempo);
}

void SequencerAdaptor::setTempoFactor(double factor)
{
    QueueTempo tempo = m_queue->getTempo();
    tempo.setTempoFactor(factor);
    m_queue->setTempo(tempo);
    m_client->drainOutput();
}

QStringList SequencerAdaptor::inputConnections()
{
    PortInfoList inputs(m_client->getAvailableInputs());
    return list_ports(inputs);
}

QStringList SequencerAdaptor::outputConnections()
{
    PortInfoList outputs(m_client->getAvailableOutputs());
    return list_ports(outputs);
}

QStringList SequencerAdaptor::list_ports(PortInfoList& refs)
{
    QStringList lst;
    foreach(PortInfo p, refs) {
        lst += QString("%1:%2").arg(p.getClientName()).arg(p.getPort());
    }
    return lst;
}

bool SequencerAdaptor::isThruEnabled() const
{
    return m_thru;
}

void SequencerAdaptor::setThruEnabled(bool enable)
{
    m_thru = enable;
}

void SequencerAdaptor::connect_input(QString name)
{
    if (!name.isEmpty())
        m_port->subscribeFrom(name);
}

void SequencerAdaptor::disconnect_input(QString name)
{
    if (!name.isEmpty())
        m_port->unsubscribeFrom(name);
}

void SequencerAdaptor::connect_output(QString name)
{
    if (!name.isEmpty())
        m_port->subscribeTo(name);
}

void SequencerAdaptor::disconnect_output(QString name)
{
    if (!name.isEmpty())
        m_port->unsubscribeTo(name);
}

QStringList SequencerAdaptor::list_subscribers()
{
    QStringList list;
    m_port->updateSubscribers();
    PortInfoList subs(m_port->getWriteSubscribers());
    PortInfoList::ConstIterator it;
    for(it = subs.constBegin(); it != subs.constEnd(); ++it) {
        PortInfo p = *it;
        list += QString("%1:%2").arg(p.getClientName()).arg(p.getPort());
    }
    return list;
}

QString SequencerAdaptor::output_subscriber()
{
    m_port->updateSubscribers();
    PortInfoList subs(m_port->getReadSubscribers());
    PortInfoList::ConstIterator it;
    for(it = subs.constBegin(); it != subs.constEnd(); ++it) {
        PortInfo p = *it;
        return QString("%1:%2").arg(p.getClientName()).arg(p.getPort());
    }
    return QString();
}

void SequencerAdaptor::disconnect_all_inputs()
{
    m_port->updateSubscribers();
    PortInfoList subs(m_port->getWriteSubscribers());
    PortInfoList::ConstIterator it;
    for(it = subs.constBegin(); it != subs.constEnd(); ++it) {
        PortInfo p = *it;
        m_port->unsubscribeFrom(&p);
    }
}

void SequencerAdaptor::connect_all_inputs()
{
    QStringList subs = list_subscribers();
    QStringList ports = inputConnections();
    QStringList::ConstIterator it;
    for ( it = ports.constBegin(); it != ports.constEnd(); ++it ) {
        if (subs.contains(*it) == 0)
            connect_input(*it);
    }
}

void SequencerAdaptor::songFinished()
{
    m_player->resetPosition();
    m_model->setCurrentRow(0);
    emit finished();
}

void SequencerAdaptor::shutupChannel(int channel)
{
    int portId = m_port->getPortId();
    ControllerEvent ev1(channel, MIDI_CTL_ALL_NOTES_OFF, 0);
    ev1.setSource(portId);
    ev1.setSubscribers();
    ev1.setDirect();
    m_client->outputDirect(&ev1);
    ControllerEvent ev2(channel, MIDI_CTL_ALL_SOUNDS_OFF, 0);
    ev2.setSource(portId);
    ev2.setSubscribers();
    ev2.setDirect();
    m_client->outputDirect(&ev2);
    m_client->drainOutput();
}

void SequencerAdaptor::shutupSound()
{
    for (int channel = 0; channel < 16; ++channel) {
        shutupChannel(channel);
    }
}

bool SequencerAdaptor::isPlaying()
{
    if (m_player != nullptr)
        return m_player->isRunning();
    return false;
}

void SequencerAdaptor::removeTrackEvents(int track)
{
    RemoveEvents spec;
    spec.setCondition(SND_SEQ_REMOVE_OUTPUT | SND_SEQ_REMOVE_TAG_MATCH);
    spec.setTag(track);
    m_client->removeEvents(&spec);
}

void SequencerAdaptor::setRequestRealtime(bool newValue)
{
    bool old = requestedRealtime();
    if (old != newValue && m_state == StoppedState) {
        m_client->stopSequencerInput();
        m_client->setRealTimeInput(newValue);
        m_client->startSequencerInput();
    }
}

bool SequencerAdaptor::requestedRealtime()
{
    return m_client->realTimeInputEnabled();
}
