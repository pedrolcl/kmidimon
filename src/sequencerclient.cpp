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

using std::cerr;
using std::endl;

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
			      "System exclusive",
			      NULL,
			      NULL,
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

MidiEvent *SequencerClient::build_channel_event( snd_seq_event_t *ev,
						 QString statusText,
						 bool useControl,
						 bool hasD2)
{
    MidiEvent *m = NULL;
    if (m_channel) {
        if (useControl) {
	    m = new MidiEvent(  event_time(ev),
				statusText,
				QString("%1").arg(ev->data.control.channel+1), 
				QString("%1").arg(ev->data.control.param), 
				hasD2 ? QString(" %1")
					    .arg(ev->data.control.value)
				      : NULL );
	} else {
	    m = new MidiEvent(  event_time(ev), 
				statusText,
				QString("%1").arg(ev->data.note.channel+1),
				QString("%1").arg(ev->data.note.note),
				QString(" %1").arg(ev->data.note.velocity) );
	}
    }
    return m;
}

MidiEvent *SequencerClient::build_bender_event(snd_seq_event_t *ev)
{
    if (m_channel) {
	return new MidiEvent( event_time(ev), 
			      "Pitch bend",
			      QString("%1").arg(ev->data.control.channel+1), 
			      QString("%1").arg(ev->data.control.value));
    }
    return NULL;
}

MidiEvent *SequencerClient::build_midi_event(snd_seq_event_t *ev)
{
    MidiEvent *me = NULL;
    switch (ev->type) {
    case SND_SEQ_EVENT_NOTEON:
	me = build_channel_event( ev, "Note on", false, true );
	break;
    case SND_SEQ_EVENT_NOTEOFF:
	me = build_channel_event( ev, "Note off", false, true );
	break;
    case SND_SEQ_EVENT_KEYPRESS:
	me = build_channel_event( ev, "Polyphonic aftertouch", false, true );
	break;
    case SND_SEQ_EVENT_CONTROLLER:
	me = build_channel_event( ev, "Control change", true, true );
	break;
    case SND_SEQ_EVENT_PGMCHANGE:
	me = build_channel_event( ev, "Program change", true, false );
	break;
    case SND_SEQ_EVENT_CHANPRESS:
	me = build_channel_event( ev, "Channel aftertouch", true, false );
	break;
    case SND_SEQ_EVENT_PITCHBEND:
	me = build_bender_event( ev );
	break;
    case SND_SEQ_EVENT_CONTROL14:
	me = build_channel_event( ev, "Control change", true, true );
	break;
    case SND_SEQ_EVENT_NONREGPARAM:
	me = build_channel_event( ev, "Non-registered parameter", true, true );
	break;
    case SND_SEQ_EVENT_REGPARAM:
	me = build_channel_event( ev, "Registered parameter", true, true );
	break;
    case SND_SEQ_EVENT_SYSEX:
	break;

    case SND_SEQ_EVENT_SONGPOS:
	if (m_common)    
	    me = new MidiEvent( event_time(ev), "Song Position");
	break;
    case SND_SEQ_EVENT_SONGSEL:
	if (m_common)    
	    me = new MidiEvent( event_time(ev), "Song Selection");
	break;
    case SND_SEQ_EVENT_QFRAME:
	if (m_common)    
	    me = new MidiEvent( event_time(ev), "Quarter Frame");
	break;
    case SND_SEQ_EVENT_TUNE_REQUEST:
	if (m_common)    
	    me = new MidiEvent( event_time(ev), "Tune Request");
	break;

    case SND_SEQ_EVENT_START:
	if (m_realtime)    
	    me = new MidiEvent( event_time(ev), "Start");
	break;
    case SND_SEQ_EVENT_CONTINUE:
	if (m_realtime)    
	    me = new MidiEvent( event_time(ev), "Continue");
	break;
    case SND_SEQ_EVENT_STOP:
	if (m_realtime)    
	    me = new MidiEvent( event_time(ev), "Stop");
	break;
    case SND_SEQ_EVENT_CLOCK:
	if (m_realtime)    
	    me = new MidiEvent( event_time(ev), "Clock");
	break;
    case SND_SEQ_EVENT_TICK:
	if (m_realtime)    
	    me = new MidiEvent( event_time(ev), "Tick");
	break;
    case SND_SEQ_EVENT_RESET:
	if (m_realtime)    
	    me = new MidiEvent( event_time(ev), "Reset");
	break;
    case SND_SEQ_EVENT_SENSING:
	if (m_realtime)    
	    me = new MidiEvent( event_time(ev), "Active Sensing");
	break;

    case SND_SEQ_EVENT_PORT_START:
	if (m_alsa)    
	  me = new MidiEvent( event_time(ev), "ALSA Port start", 
			      NULL,
			      event_addr(ev) );
	break;
    case SND_SEQ_EVENT_PORT_EXIT:
	if (m_alsa)    
	  me = new MidiEvent( event_time(ev), "ALSA Port exit", 
			      NULL,
			      event_addr(ev) );
	break;
    case SND_SEQ_EVENT_PORT_CHANGE:
	if (m_alsa)    
	  me = new MidiEvent( event_time(ev), "ALSA Port change", 
			      NULL,
			      event_addr(ev) );
	break;
    case SND_SEQ_EVENT_CLIENT_START:
	if (m_alsa)    
	  me = new MidiEvent( event_time(ev), "ALSA Client start", 
			      NULL,
			      event_addr(ev) );
	break;
    case SND_SEQ_EVENT_CLIENT_EXIT:
	if (m_alsa)    
	  me = new MidiEvent( event_time(ev), "ALSA Client exit", 
			      NULL,
			      event_addr(ev) );
	break;
    case SND_SEQ_EVENT_CLIENT_CHANGE:
	if (m_alsa)    
	  me = new MidiEvent( event_time(ev), "ALSA Client change", 
			      NULL,
			      event_addr(ev) );
	break;
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
	if (m_alsa)    
	  me = new MidiEvent( event_time(ev), "ALSA Port subscribed", 
			      NULL,
			      event_sender(ev),
			      event_dest(ev) );
	break;
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
	if (m_alsa)    
	  me = new MidiEvent( event_time(ev), "ALSA Port unsubscribed", 
			      NULL,
			      event_sender(ev),
			      event_dest(ev) );
	break;
	
    default:
	  me = new MidiEvent( event_time(ev),
			      QString("Event type %1").arg(ev->type));
    }
    
    return me;
}
