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

#include <iostream>
#include <stdexcept>
#include <QStringList>
#include <QDebug>

#include <kapplication.h>
#include <klocale.h>

#include <client.h>
#include <port.h>
#include <queue.h>
#include <subscription.h>
#include <event.h>

#include "sequenceradaptor.h"
#include "sequenceitem.h"
#include "sequencemodel.h"

using namespace std;

SequencerAdaptor::SequencerAdaptor(QObject *parent):
    QObject(parent),
    m_queue_running(false),
    m_resolution(RESOLUTION),
    m_tempo(TEMPO_BPM)
{
    m_client = new MidiClient(this);
    m_client->setOpenMode(SND_SEQ_OPEN_DUPLEX);
    m_client->setBlockMode(false);
    m_client->open();
    m_client->setClientName("KMidimon");
    m_clientId = m_client->getClientId();
    connect(m_client, SIGNAL(eventReceived(SequencerEvent*)),
            SLOT(sequencerEvent(SequencerEvent*)));

    m_queue = m_client->createQueue("KMidimon");
    m_queueId = m_queue->getId();

    m_port = new MidiPort(this);
    m_port->setMidiClient(m_client);
    m_port->setPortName("KMidimon Input");
    m_port->setCapability( SND_SEQ_PORT_CAP_WRITE |
                           SND_SEQ_PORT_CAP_SUBS_WRITE );
    m_port->setPortType( SND_SEQ_PORT_TYPE_MIDI_GENERIC |
                         SND_SEQ_PORT_TYPE_APPLICATION );
    m_port->setMidiChannels(16);
    m_port->setTimestamping(true);
    m_port->setTimestampReal(true);
    m_port->setTimestampQueue(m_queueId);
    m_port->attach();
    m_port->subscribeFromAnnounce();
    m_client->startSequencerInput();
}

SequencerAdaptor::~SequencerAdaptor()
{
    m_client->stopSequencerInput();
    m_port->detach();
    m_client->close();
}

void SequencerAdaptor::change_port_settings()
{
    //m_port->setTimestampReal(!m_tickTimeFilter);
    m_port->setTimestampReal(false);
}

void SequencerAdaptor::sequencerEvent(SequencerEvent* ev)
{
    if (m_queue_running)
        //if ev->isClient()
        //   m_model->updateClientsList();
        m_model->addItem(ev);
    else
        delete ev;
}

void SequencerAdaptor::queue_start()
{
    m_queue->start();
    m_queue_running = m_queue->getStatus().isRunning();
}

void SequencerAdaptor::queue_stop()
{
    m_queue->stop();
    m_queue_running = m_queue->getStatus().isRunning();
}

void SequencerAdaptor::queue_set_tempo()
{
    QueueTempo tempo = m_queue->getTempo();
    tempo.setPPQ(m_resolution);
    tempo.setNominalBPM(m_tempo);
    m_queue->setTempo(tempo);
}

QStringList SequencerAdaptor::inputConnections()
{
    PortInfoList inputs(m_client->getAvailableInputs());
    return list_ports(inputs);
}

QStringList SequencerAdaptor::list_ports(PortInfoList& refs)
{
    QStringList lst;
    foreach(PortInfo p, refs) {
        lst += QString("%1:%2").arg(p.getClientName()).arg(p.getPort());
    }
    return lst;
}

void SequencerAdaptor::connect_port(QString name)
{
    //qDebug() << "connecting: " << name;
    m_port->subscribeFrom(name);
}

void SequencerAdaptor::disconnect_port(QString name)
{
    //qDebug() << "disconnecting: " << name;
    m_port->unsubscribeFrom(name);
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

void SequencerAdaptor::disconnect_all()
{
    m_port->unsubscribeAll();
}

void SequencerAdaptor::connect_all()
{
    QStringList subs = list_subscribers();
    QStringList ports = inputConnections();
    QStringList::ConstIterator it;
    for ( it = ports.constBegin(); it != ports.constEnd(); ++it ) {
        if (subs.contains(*it) == 0)
            connect_port(*it);
    }
}
