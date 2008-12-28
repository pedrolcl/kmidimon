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

using namespace std;

SequencerAdaptor::SequencerAdaptor(QObject *parent) : QObject(parent),
	m_queue_running(false),
	m_resolution(RESOLUTION),
	m_tempo(TEMPO_BPM),
	m_tickTime(true),
	m_channel(true),
	m_common(true),
	m_realtime(true),
	m_sysex(true),
	m_alsa(true),
	m_showClientNames(false),
	m_translateSysex(false),
	m_needsRefresh(true)
{
	m_client = new MidiClient(this);
	m_port = new MidiPort(this);

	m_client->setOpenMode(SND_SEQ_OPEN_DUPLEX);
    m_client->setBlockMode(false);
    m_client->open();
	m_client->setClientName("KMidimon");
    m_clientId = m_client->getClientId();
    connect(m_client, SIGNAL(eventReceived(SequencerEvent*)),
                      SLOT(sequencerEvent(SequencerEvent*)));

    m_queue = m_client->createQueue("KMidimon");
	m_queueId = m_queue->getId();

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
	m_port->setTimestampReal(!m_tickTime);
}

void SequencerAdaptor::sequencerEvent(SequencerEvent* ev)
{
	if (m_queue_running) {
		MidiEvent *me = build_midi_event(ev);
		if (me != NULL) {
			KApplication::postEvent(parent(), me);
		}
	}
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

MidiEvent *SequencerAdaptor::build_translated_sysex(SysExEvent *ev)
{
    int val, cmd;
    unsigned int i, len;
    unsigned char *ptr = (unsigned char *) ev->getData();
    if (ev->getLength() < 6) return NULL;
    if (*ptr++ != 0xf0) return NULL;
    int msgId = *ptr++;
    int deviceId = *ptr++;
    int subId1 = *ptr++;
    int subId2 = *ptr++;
    QString data1 = QString::null, data2 = QString::null;
    QString devid( deviceId == 0x7f ? i18n("broadcast") : i18n("device %1").arg(deviceId) );
    if (msgId == 0x7e) { // Universal Non Real Time
        switch (subId1) {
            case 0x01:
                data1 = i18n("Sample Dump");
                data2 = i18n("Header");
                break;
            case 0x02:
                data1 = i18n("Sample Dump");
                data2 = i18n("Data Packet");
                break;
            case 0x03:
                data1 = i18n("Sample Dump");
                data2 = i18n("Request");
                break;
            case 0x04:
                data1 = i18n("MTC Setup");
                switch (subId2) {
                    case 0x00:
                        data2 = i18n("Special");
                        break;
                    case 0x01:
                        data2 = i18n("Punch In Points");
                        break;
                    case 0x02:
                        data2 = i18n("Punch Out Points");
                        break;
                    case 0x03:
                        data2 = i18n("Delete Punch In Points");
                        break;
                    case 0x04:
                        data2 = i18n("Delete Punch Out Points");
                        break;
                    case 0x05:
                        data2 = i18n("Event Start Point");
                        break;
                    case 0x06:
                        data2 = i18n("Event Stop Point");
                        break;
                    case 0x07:
                        data2 = i18n("Event Start Point With Info");
                        break;
                    case 0x08:
                        data2 = i18n("Event Stop Point With Info");
                        break;
                    case 0x09:
                        data2 = i18n("Delete Event Start Point");
                        break;
                    case 0x0a:
                        data2 = i18n("Delete Event Stop Point");
                        break;
                    case 0x0b:
                        data2 = i18n("Cue Points");
                        break;
                    case 0x0c:
                        data2 = i18n("Cue Points With Info");
                        break;
                    case 0x0d:
                        data2 = i18n("Delete Cue Points");
                        break;
                    case 0x0e:
                        data2 = i18n("Event Name");
                        break;
                    default:
                        return NULL;
                }
                break;
            case 0x05:
                data1 = i18n("Sample Dump");
                switch (subId2) {
                    case 0x01:
                        data2 = i18n("Multiple Loop Points");
                        break;
                    case 0x02:
                        data2 = i18n("Loop Points Request");
                        break;
                    default:
                        return NULL;
                }
                break;
            case 0x06:
                data1 = i18n("Gen.Info");
                switch (subId2) {
                    case 0x01:
                        data2 = i18n("Identity Request");
                        break;
                    case 0x02:
                        if (ev->getLength() < 15) return NULL;
                        data2 = i18n("Identity Reply: %1 %2 %3 %4 %5 %6 %7 %8 %9")
                                .arg(ptr[0]).arg(ptr[1]).arg(ptr[2]).arg(ptr[3])
                                .arg(ptr[4]).arg(ptr[5]).arg(ptr[6]).arg(ptr[7])
                                .arg(ptr[8]);
                        break;
                    default:
                        return NULL;
                }
                break;
            case 0x08:
                data1 = i18n("Tuning");
                if (ev->getLength() < 7) return NULL;
                switch (subId2) {
                    case 0x00:
                        data2 = i18n("Dump Request: %1").arg(*ptr++);
                        break;
                    case 0x01:
                        data2 = i18n("Bulk Dump: %1").arg(*ptr++);
                        break;
                    case 0x02:
                        data2 = i18n("Note Change: %1").arg(*ptr++);
                        break;
                    default:
                        return NULL;
                }
                break;
            case 0x09:
                data1 = i18n("GM Mode");
                switch (subId2) {
                    case 0x01:
                        data2 = i18n("GM On");
                        break;
                    case 0x02:
                        data2 = i18n("GM Off");
                        break;
                    default:
                        return NULL;
                }
                break;
            case 0x7c:
                data1 = i18n("Wait");
                break;
            case 0x7d:
                data1 = i18n("Cancel");
                break;
            case 0x7e:
                data1 = i18n("NAK");
                break;
            case 0x7f:
                data1 = i18n("ACK");
                break;
            default:
                return NULL;
        }
        return new MidiEvent( event_time(ev),
                              event_source(ev),
                              i18n("Universal Non Real Time SysEx"),
                              devid,
                              data1,
                              data2 );
    } else
    if (msgId == 0x7f) { // Universal Real Time
        switch (subId1) {
            case 0x01:
                data1 = i18n("MTC");
                switch (subId2) {
                    case 0x01:
                        if (ev->getLength() < 10) return NULL;
                        data2 = i18n("Full Frame: %1 %2 %3 %4")
                                .arg(ptr[0]).arg(ptr[1]).arg(ptr[2]).arg(ptr[3]);
                        break;
                    case 0x02:
                        if (ev->getLength() < 15) return NULL;
                        data2 = i18n("User Bits: %1 %2 %3 %4 %5 %6 %7 %8 %9")
                                .arg(ptr[0]).arg(ptr[1]).arg(ptr[2]).arg(ptr[3])
                                .arg(ptr[4]).arg(ptr[5]).arg(ptr[6]).arg(ptr[7])
                                .arg(ptr[8]);
                        break;
                    default:
                        return NULL;
                }
                break;
            case 0x03:
                data1 = i18n("Notation");
                switch (subId2) {
                    case 0x01:
                        if (ev->getLength() < 8) return NULL;
                        val = *ptr++;
                        val += (*ptr++ * 128);
                        data2 = i18n("Bar Marker: %1").arg(val - 8192);
                        break;
                    case 0x02:
                    case 0x42:
                        if (ev->getLength() < 9) return NULL;
                        data2 = i18n("Time Signature: %1 (%2/%3)")
                                .arg(ptr[0]).arg(ptr[1]).arg(ptr[2]);
                        break;
                    default:
                        return NULL;
                }
                break;
            case 0x04:
                data1 = i18n("GM Master");
                if (ev->getLength() < 8) return NULL;
                val = *ptr++;
                val += (*ptr++ * 128);
                switch (subId2) {
                    case 0x01:
                        data2 = i18n("Volume: %1").arg(val);
                        break;
                    case 0x02:
                        data2 = i18n("Balance: %1").arg(val - 8192);
                        break;
                    default:
                        return NULL;
                }
                break;
	        case 0x06:
                data1 = i18n("MMC");
    	        switch (subId2) {
    	            case 0x01:
    	                data2 = i18n("Stop");
    	                break;
    				case 0x02:
    	                data2 = i18n("Play");
    	                break;
    				case 0x03:
    	                data2 = i18n("Deferred Play");
    	                break;
    				case 0x04:
    	                data2 = i18n("Fast Forward");
    	                break;
    				case 0x05:
    	                data2 = i18n("Rewind");
    	                break;
    				case 0x06:
    	                data2 = i18n("Punch In");
    	                break;
    				case 0x07:
    	                data2 = i18n("Punch out");
    	                break;
    				case 0x09:
    	                data2 = i18n("Pause");
    	                break;
                    case 0x0a:
                        data2 = i18n("Eject");
                        break;
                    case 0x40:
                        len = *ptr++;
                        if (ev->getLength() < (7+len)) return NULL;
                        cmd = *ptr++;
                        switch (cmd) {
                            case 0x4f:
                                data2 = i18n("Track Record Ready:");
                                break;
                            case 0x52:
                                data2 = i18n("Track Sync Monitor:");
                                break;
                            case 0x53:
                                data2 = i18n("Track Input Monitor:");
                                break;
                            case 0x62:
                                data2 = i18n("Track Mute:");
                                break;
                            default:
                                return NULL;
                        }
                        for (i=1; i < len; ++i) {
                            data2.append(QString(" %1").arg(*ptr++, 0, 16));
                        }
                        break;
                    case 0x44:
                        if (ev->getLength() < 13) return NULL;
                        *ptr++; *ptr++;
                        data2 = i18n("Locate: %1 %2 %3 %4 %5")
                                .arg(ptr[0]).arg(ptr[1]).arg(ptr[2]).arg(ptr[3])
                                .arg(ptr[4]);
                        break;
                    default:
                        return NULL;
    	        }
                break;
            default:
                return NULL;
        }
        return new MidiEvent( event_time(ev),
                              event_source(ev),
                              i18n("Universal Real Time SysEx"),
                              devid,
                              data1,
                              data2 );
    }
    return NULL;
}

MidiEvent *SequencerAdaptor::build_sysex_event(SysExEvent *ev)
{
    if (m_sysex) {
        MidiEvent *me;
        if (m_translateSysex) {
            me = build_translated_sysex(ev);
            if (me != NULL) return me;
        }
		unsigned int i;
		unsigned char *data = (unsigned char *) ev->getData();
		QString text;
		for (i = 0; i < ev->getLength(); ++i) {
			QString h(QString("%1").arg(data[i], 0, 16));
			if (h.length() < 2) h.prepend('0');
		    h.prepend(' ');
		    text.append(h);
		}
		return new MidiEvent( event_time(ev),
				      event_source(ev),
				      i18n("System exclusive"),
				      NULL,
				      QString("%1").arg(ev->getLength()),
				      text );
    }
    return NULL;
}

QString SequencerAdaptor::event_time(SequencerEvent *ev)
{
    if (m_tickTime) {
		return QString("%1 ").arg(ev->getTick());
    } else {
    	QString d = QString::number(ev->getRealTimeNanos()/1000, 'f', 0);
    	d = d.left(4).leftJustified(4, '0');
		return QString("%1.%2 ").arg(ev->getRealTimeSecs()).arg(d);
    }
}

QString SequencerAdaptor::client_name(int client_number)
{
    if (showClientNames()) {
     	return m_client->getClientName(client_number);
    }
    return QString("%1").arg(client_number);
}

QString SequencerAdaptor::event_source(SequencerEvent *ev)
{
    return QString("%1:%2").arg(client_name(ev->getSourceClient()))
    			           .arg(ev->getSourcePort());
}

QString SequencerAdaptor::event_addr(SequencerEvent *ev)
{
    return QString("%1:%2").arg(client_name(ev->getHandle()->data.addr.client))
    			   .arg(ev->getHandle()->data.addr.port);
}

QString SequencerAdaptor::event_client(SequencerEvent *ev)
{
    return client_name(ev->getHandle()->data.addr.client);
}

QString SequencerAdaptor::event_sender(SequencerEvent *ev)
{
    return QString("%1:%2").arg(client_name(ev->getHandle()->data.connect.sender.client))
    			   .arg(ev->getHandle()->data.connect.sender.port);
}

QString SequencerAdaptor::event_dest(SequencerEvent *ev)
{
    return QString(" %1:%2").arg(client_name(ev->getHandle()->data.connect.dest.client))
    			   .arg(ev->getHandle()->data.connect.dest.port);
}

QString SequencerAdaptor::common_param(SequencerEvent *ev)
{
    return QString("%1").arg(ev->getHandle()->data.control.value);
}

MidiEvent *SequencerAdaptor::build_control_event(ControllerEvent *ev,
						 QString statusText )
{
    if (m_channel) {
		return  new MidiEvent(  event_time(ev),
					event_source(ev),
					statusText,
					QString("%1").arg(ev->getChannel()+1),
					QString("%1").arg(ev->getParam()),
					QString(" %1").arg(ev->getValue()));
    }
    return NULL;
}

MidiEvent *SequencerAdaptor::build_note_event( KeyEvent *ev,
						 QString statusText )
{
    if (m_channel) {
		return  new MidiEvent(event_time(ev),
						event_source(ev),
						statusText,
						QString("%1").arg(ev->getChannel()+1),
						QString("%1").arg(ev->getKey()),
						QString(" %1").arg(ev->getVelocity()));
    }
    return NULL;
}

MidiEvent *SequencerAdaptor::build_common_event( SequencerEvent *ev,
						QString statusText,
						QString param )
{
    if (m_common) {
		return new MidiEvent( event_time(ev),
				      event_source(ev),
				      statusText, NULL, param );
    }
    return NULL;
}

MidiEvent *SequencerAdaptor::build_realtime_event( SequencerEvent *ev,
						  QString statusText )
{
    if (m_realtime) {
		return new MidiEvent( event_time(ev), event_source(ev), statusText);
    }
    return NULL;
}

MidiEvent *SequencerAdaptor::build_alsa_event( SequencerEvent *ev,
					      QString statusText,
					      QString srcAddr,
					      QString dstAddr )
{
    if (m_alsa) {
		return new MidiEvent( event_time(ev),
				      event_source(ev),
				      statusText,
				      NULL, srcAddr, dstAddr );
    }
    return NULL;
}

MidiEvent *SequencerAdaptor::build_controlv_event(SequencerEvent *ev,
						  QString statusText )
{
    if (m_channel) {
    	ChannelEvent* cev = static_cast<ChannelEvent*>(ev);
    	int value = cev->getHandle()->data.control.value;
		return new MidiEvent( event_time(ev),
				      event_source(ev),
				      statusText,
				      QString("%1").arg(cev->getChannel()+1),
				      QString("%1").arg(value));
    }
    return NULL;
}

MidiEvent *SequencerAdaptor::build_midi_event(SequencerEvent *ev)
{
    MidiEvent *me = NULL;
    switch (ev->getSequencerType()) {
		/* MIDI Channel events */
	    case SND_SEQ_EVENT_NOTEON:
			me = build_note_event(static_cast<KeyEvent*>(ev), i18n("Note on"));
			break;
	    case SND_SEQ_EVENT_NOTEOFF:
			me = build_note_event(static_cast<KeyEvent*>(ev), i18n("Note off"));
			break;
	    case SND_SEQ_EVENT_KEYPRESS:
			me = build_note_event(static_cast<KeyEvent*>(ev), i18n("Polyphonic aftertouch"));
			break;
	    case SND_SEQ_EVENT_CONTROLLER:
			me = build_control_event(static_cast<ControllerEvent*>(ev), i18n("Control change"));
			break;
	    case SND_SEQ_EVENT_PGMCHANGE:
			me = build_controlv_event(ev, i18n("Program change"));
			break;
	    case SND_SEQ_EVENT_CHANPRESS:
			me = build_controlv_event(ev, i18n("Channel aftertouch"));
			break;
	    case SND_SEQ_EVENT_PITCHBEND:
			me = build_controlv_event(ev, i18n("Pitch bend"));
			break;
	    case SND_SEQ_EVENT_CONTROL14:
			me = build_control_event(static_cast<ControllerEvent*>(ev), i18n("Control change"));
			break;
	    case SND_SEQ_EVENT_NONREGPARAM:
			me = build_control_event(static_cast<ControllerEvent*>(ev), i18n("Non-registered parameter"));
			break;
	    case SND_SEQ_EVENT_REGPARAM:
			me = build_control_event(static_cast<ControllerEvent*>(ev), i18n("Registered parameter"));
			break;
		/* MIDI Common events */
	    case SND_SEQ_EVENT_SYSEX:
			me = build_sysex_event(static_cast<SysExEvent*>(ev));
			break;
	    case SND_SEQ_EVENT_SONGPOS:
			me = build_common_event(ev, i18n("Song Position"), common_param(ev));
			break;
	    case SND_SEQ_EVENT_SONGSEL:
			me = build_common_event(ev, i18n("Song Selection"), common_param(ev));
			break;
	    case SND_SEQ_EVENT_QFRAME:
			me = build_common_event(ev, i18n("MTC Quarter Frame"), common_param(ev));
			break;
	    case SND_SEQ_EVENT_TUNE_REQUEST:
			me = build_common_event(ev, i18n("Tune Request"));
			break;
		/* MIDI Realtime Events */
	    case SND_SEQ_EVENT_START:
			me = build_realtime_event(ev, i18n("Start"));
			break;
	    case SND_SEQ_EVENT_CONTINUE:
			me = build_realtime_event(ev, i18n("Continue"));
			break;
	    case SND_SEQ_EVENT_STOP:
			me = build_realtime_event(ev, i18n("Stop"));
			break;
	    case SND_SEQ_EVENT_CLOCK:
			me = build_realtime_event(ev, i18n("Clock"));
			break;
	    case SND_SEQ_EVENT_TICK:
			me = build_realtime_event(ev, i18n("Tick"));
			break;
	    case SND_SEQ_EVENT_RESET:
			me = build_realtime_event(ev, i18n("Reset"));
			break;
	    case SND_SEQ_EVENT_SENSING:
			me = build_realtime_event(ev, i18n("Active Sensing"));
			break;
		/* ALSA Client/Port events */
	    case SND_SEQ_EVENT_PORT_START:
	        m_needsRefresh = true;
			me = build_alsa_event(ev, i18n("ALSA Port start"), event_addr(ev));
			break;
	    case SND_SEQ_EVENT_PORT_EXIT:
			me = build_alsa_event(ev, i18n("ALSA Port exit"), event_addr(ev));
			break;
	    case SND_SEQ_EVENT_PORT_CHANGE:
			m_needsRefresh = true;
			me = build_alsa_event(ev, i18n("ALSA Port change"), event_addr(ev));
			break;
	    case SND_SEQ_EVENT_CLIENT_START:
			m_needsRefresh = true;
			me = build_alsa_event(ev, i18n("ALSA Client start"), event_client(ev));
			break;
	    case SND_SEQ_EVENT_CLIENT_EXIT:
			me = build_alsa_event(ev, i18n("ALSA Client exit"), event_client(ev));
			break;
	    case SND_SEQ_EVENT_CLIENT_CHANGE:
			m_needsRefresh = true;
			me = build_alsa_event(ev, i18n("ALSA Client change"), event_client(ev));
			break;
	    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
			me = build_alsa_event(ev, i18n("ALSA Port subscribed"),
						event_sender(ev), event_dest(ev));
			break;
	    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
			me = build_alsa_event (ev, i18n("ALSA Port unsubscribed"),
						event_sender(ev), event_dest(ev));
			break;
		/* Other events */
	    default:
			me = new MidiEvent(event_time(ev),
							   event_source(ev),
							   QString("Event type %1").arg(ev->getSequencerType()));
    }
    return me;
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
