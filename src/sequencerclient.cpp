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
#include <iostream>
#include <qstringlist.h>
#include <kapplication.h>
#include <klocale.h>
#include <alsa/asoundlib.h>
#include "sequencerclient.h"

using namespace std;

SequencerClient::SequencerClient(QWidget *parent):QThread(),
	m_widget(parent),
	m_queue_running(false),
	m_resolution(RESOLUTION),
	m_tempo(TEMPO_BPM),
	m_tickTime(true),
	m_channel(true),
	m_common(true),
	m_realtime(true),
	m_sysex(true),
	m_alsa(true)
{
    int err;
    snd_seq_port_info_t *pinfo;
    
    err = snd_seq_open( &m_handle, 
    			"default", 
    			SND_SEQ_OPEN_DUPLEX, 
    			SND_SEQ_NONBLOCK );
    checkAlsaError(err, "Sequencer open");
    m_client = snd_seq_client_id(m_handle);
    snd_seq_set_client_name(m_handle, "KMidimon");

    m_queue = snd_seq_alloc_named_queue(m_handle, "KMidiMon_queue");
    checkAlsaError( m_queue, "Creating queue" );

    snd_seq_port_info_alloca(&pinfo);
    snd_seq_port_info_set_capability( pinfo,
				      SND_SEQ_PORT_CAP_WRITE |
				      SND_SEQ_PORT_CAP_SUBS_WRITE );
    snd_seq_port_info_set_type( pinfo,
				SND_SEQ_PORT_TYPE_MIDI_GENERIC |
				SND_SEQ_PORT_TYPE_APPLICATION );
    snd_seq_port_info_set_midi_channels(pinfo, 16);
    snd_seq_port_info_set_timestamping(pinfo, 1);
    snd_seq_port_info_set_timestamp_real(pinfo, 1);    
    snd_seq_port_info_set_timestamp_queue(pinfo, m_queue);
    snd_seq_port_info_set_name(pinfo, "input");
    m_input = snd_seq_create_port(m_handle, pinfo);
    checkAlsaError( m_input, "Creating input port" );
   
    err = snd_seq_connect_from( m_handle, 
				m_input, 
				SND_SEQ_CLIENT_SYSTEM, 
				SND_SEQ_PORT_SYSTEM_ANNOUNCE );
    checkAlsaError(err, "connecting from system::announce");
    start();
}

SequencerClient::~SequencerClient()
{
    int err;
    queue_stop();
    err = snd_seq_free_queue(m_handle, m_queue);
    checkAlsaError(err, "Freeing queue");
    err = snd_seq_close(m_handle);
    checkAlsaError(err, "Closing");
}

void SequencerClient::change_port_settings() 
{
    snd_seq_port_info_t *pinfo;
    snd_seq_port_info_alloca(&pinfo);
    checkAlsaError( snd_seq_get_port_info(m_handle, m_input, pinfo),
		    "get_port_info" );
    snd_seq_port_info_set_timestamp_real(pinfo, m_tickTime ? 0 : 1);            
    checkAlsaError( snd_seq_set_port_info(m_handle, m_input, pinfo),
		    "set_port_info" );    
}

int SequencerClient::checkAlsaError(int rc, const char *message) 
{
    if (rc < 0) {
	cerr << "KMidiMon ALSA Error " << message << " rc: " << rc
	     << " (" << snd_strerror(rc) << ")" << endl;
    }
    return rc;
}

void SequencerClient::run()
{
    struct pollfd *pfds;
    int npfds;
    int rt, err = 0;
    npfds = snd_seq_poll_descriptors_count(m_handle, POLLIN);
    pfds = (pollfd *)alloca(sizeof(*pfds) * npfds);
    snd_seq_poll_descriptors(m_handle, pfds, npfds, POLLIN);
    while(true) {
	rt = poll(pfds, npfds, 1000);
	if (rt >= 0)
	do {
	    snd_seq_event_t *ev;
	    err = snd_seq_event_input(m_handle, &ev);
	    if (err >= 0 && ev) {
	    	if (m_queue_running) {
	    	    MidiEvent *me = build_midi_event(ev);
	    	    if (me != NULL) {
			KApplication::postEvent(m_widget, me);
	    	    }
	    	}
	    }
	} while (snd_seq_event_input_pending(m_handle, 0) > 0);
    }
}

void SequencerClient::queue_start()
{
    int err = 0;
    err = checkAlsaError( snd_seq_start_queue(m_handle, m_queue, NULL), 
			  "start_queue" );
    err = checkAlsaError( snd_seq_drain_output(m_handle), 
			  "drain_output (queue_start)" );
    m_queue_running = (err == 0);
}

void SequencerClient::queue_stop()
{
    int err = 0;
    err = checkAlsaError( snd_seq_stop_queue(m_handle, m_queue, NULL), 
			  "stop_queue" );
    err = checkAlsaError( snd_seq_drain_output(m_handle), 
			  "drain_output (queue_stop)" );
    m_queue_running = !(err == 0);
}

void SequencerClient::queue_set_tempo()
{
    snd_seq_queue_tempo_t *qtempo;
    int tempo = (int) (6e7 / m_tempo);
    snd_seq_queue_tempo_alloca(&qtempo);
    snd_seq_queue_tempo_set_tempo(qtempo, tempo);
    snd_seq_queue_tempo_set_ppq(qtempo, m_resolution);
    checkAlsaError( snd_seq_set_queue_tempo(m_handle, m_queue, qtempo), 
    		    "set_queue_tempo");
    checkAlsaError( snd_seq_drain_output(m_handle), 
                    "drain_output (set_tempo)");
}

MidiEvent *SequencerClient::build_sysex_event(snd_seq_event_t *ev)
{
    if (m_sysex) {
	unsigned int i;
	unsigned char *data = (unsigned char *)ev->data.ext.ptr;
	QString text; 
	for (i = 0; i < ev->data.ext.len; ++i) {
	    text.append(QString(" %1").arg(data[i], 0, 16));
	}
	return new MidiEvent( event_time(ev), 
			      i18n("System exclusive"),
			      NULL,
			      QString("%1").arg(ev->data.ext.len),
			      text );
    } 
    return NULL;
}

QString SequencerClient::event_time(snd_seq_event_t *ev)
{
    if (m_tickTime)
	return QString("%1 ").arg(ev->time.tick);
    else
	return QString("%1.%2 ").arg(ev->time.time.tv_sec)
				.arg(ev->time.time.tv_nsec/1000);
}

QString SequencerClient::event_addr(snd_seq_event_t *ev)
{
    return QString("%1:%2").arg(ev->data.addr.client)
    			   .arg(ev->data.addr.port);
}

QString SequencerClient::event_sender(snd_seq_event_t *ev)
{
    return QString("%1:%2").arg(ev->data.connect.sender.client)
    			   .arg(ev->data.connect.sender.port);
}

QString SequencerClient::event_dest(snd_seq_event_t *ev)
{
    return QString(" %1:%2").arg(ev->data.connect.dest.client)
    			   .arg(ev->data.connect.dest.port);
}

QString SequencerClient::common_param(snd_seq_event_t *ev)
{
    return QString("%1").arg(ev->data.control.value);
}

MidiEvent *SequencerClient::build_control_event( snd_seq_event_t *ev,
						 QString statusText )
{
    if (m_channel) {
	return  new MidiEvent(  event_time(ev),
				statusText,
				QString("%1").arg(ev->data.control.channel+1), 
				QString("%1").arg(ev->data.control.param), 
				QString(" %1").arg(ev->data.control.value) );
    }
    return NULL;
}

MidiEvent *SequencerClient::build_note_event( snd_seq_event_t *ev,
						 QString statusText )
{
    if (m_channel) {
	return  new MidiEvent(  event_time(ev), 
				statusText,
				QString("%1").arg(ev->data.note.channel+1),
				QString("%1").arg(ev->data.note.note),
				QString(" %1").arg(ev->data.note.velocity) );
    }
    return NULL;
}

MidiEvent *SequencerClient::build_common_event( snd_seq_event_t *ev,
						QString statusText,
						QString param )
{
    if (m_common) {
	return new MidiEvent( event_time(ev), statusText, NULL, param );
    }
    return NULL;
}

MidiEvent *SequencerClient::build_realtime_event( snd_seq_event_t *ev,
						  QString statusText )
{
    if (m_realtime) {
	return new MidiEvent( event_time(ev), statusText);
    }
    return NULL;
}

MidiEvent *SequencerClient::build_alsa_event( snd_seq_event_t *ev,
					      QString statusText,
					      QString srcAddr,
					      QString dstAddr )
{
    if (m_alsa) {
	return new MidiEvent( event_time(ev), statusText, 
			      NULL, srcAddr, dstAddr );
    }
    return NULL;
}

MidiEvent *SequencerClient::build_controlv_event( snd_seq_event_t *ev, 
						  QString statusText )
{
    if (m_channel) {
	return new MidiEvent( event_time(ev), 
			      statusText,
			      QString("%1").arg(ev->data.control.channel+1), 
			      QString("%1").arg(ev->data.control.value));
    }
    return NULL;
}

MidiEvent *SequencerClient::build_midi_event(snd_seq_event_t *ev)
{
    MidiEvent *me = NULL;
    switch (ev->type) {
/* MIDI Channel events */    	
    case SND_SEQ_EVENT_NOTEON:
	me = build_note_event( ev, i18n("Note on") );
	break;
    case SND_SEQ_EVENT_NOTEOFF:
	me = build_note_event( ev, i18n("Note off") );
	break;
    case SND_SEQ_EVENT_KEYPRESS:
	me = build_note_event( ev, i18n("Polyphonic aftertouch") );
	break;
    case SND_SEQ_EVENT_CONTROLLER:
	me = build_control_event( ev, i18n("Control change") );
	break;
    case SND_SEQ_EVENT_PGMCHANGE:
	me = build_controlv_event( ev, i18n("Program change") );
	break;
    case SND_SEQ_EVENT_CHANPRESS:
	me = build_controlv_event( ev, i18n("Channel aftertouch") );
	break;
    case SND_SEQ_EVENT_PITCHBEND:
	me = build_controlv_event( ev, i18n("Pitch bend") );
	break;
    case SND_SEQ_EVENT_CONTROL14:
	me = build_control_event( ev, i18n("Control change") );
	break;
    case SND_SEQ_EVENT_NONREGPARAM:
	me = build_control_event( ev, i18n("Non-registered parameter") );
	break;
    case SND_SEQ_EVENT_REGPARAM:
	me = build_control_event( ev, i18n("Registered parameter") );
	break;
/* MIDI Common events */
    case SND_SEQ_EVENT_SYSEX:
	me = build_sysex_event( ev );    
	break;
    case SND_SEQ_EVENT_SONGPOS:
	me = build_common_event( ev, i18n("Song Position"), common_param(ev) );
	break;
    case SND_SEQ_EVENT_SONGSEL:
	me = build_common_event( ev, i18n("Song Selection"), common_param(ev) );
	break;
    case SND_SEQ_EVENT_QFRAME:
	me = build_common_event( ev, i18n("Quarter Frame"), common_param(ev) );
	break;
    case SND_SEQ_EVENT_TUNE_REQUEST:
	me = build_common_event( ev, i18n("Tune Request") );
	break;
/* MIDI Realtime Events */
    case SND_SEQ_EVENT_START:
	me = build_realtime_event( ev, i18n("Start") );
	break;
    case SND_SEQ_EVENT_CONTINUE:
	me = build_realtime_event( ev, i18n("Continue") );
	break;
    case SND_SEQ_EVENT_STOP:
	me = build_realtime_event( ev, i18n("Stop") );
	break;
    case SND_SEQ_EVENT_CLOCK:
	me = build_realtime_event( ev, i18n("Clock") );
	break;
    case SND_SEQ_EVENT_TICK:
	me = build_realtime_event( ev, i18n("Tick") );
	break;
    case SND_SEQ_EVENT_RESET:
	me = build_realtime_event( ev, i18n("Reset") );
	break;
    case SND_SEQ_EVENT_SENSING:
	me = build_realtime_event( ev, i18n("Active Sensing") );
	break;
/* ALSA Client/Port events */
    case SND_SEQ_EVENT_PORT_START:
	me = build_alsa_event( ev, i18n("ALSA Port start"), event_addr(ev) );
	break;
    case SND_SEQ_EVENT_PORT_EXIT:
	me = build_alsa_event( ev, i18n("ALSA Port exit"), event_addr(ev) );
	break;
    case SND_SEQ_EVENT_PORT_CHANGE:
	me = build_alsa_event( ev, i18n("ALSA Port change"), event_addr(ev) );
	break;
    case SND_SEQ_EVENT_CLIENT_START:
	me = build_alsa_event ( ev, i18n("ALSA Client start"), event_addr(ev) );
	break;
    case SND_SEQ_EVENT_CLIENT_EXIT:
	me = build_alsa_event ( ev, i18n("ALSA Client exit"), event_addr(ev) );
	break;
    case SND_SEQ_EVENT_CLIENT_CHANGE:
	me = build_alsa_event ( ev, i18n("ALSA Client change"), event_addr(ev) );
	break;
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
	me = build_alsa_event ( ev, i18n("ALSA Port subscribed"),     
				event_sender(ev), event_dest(ev) );
	break;
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
	me = build_alsa_event ( ev, i18n("ALSA Port unsubscribed"),     
				event_sender(ev), event_dest(ev) );
	break;
/* Other events */	
    default:
	  me = new MidiEvent( event_time(ev),
			      QString("Event type %1").arg(ev->type));
    }
    
    return me;
}
