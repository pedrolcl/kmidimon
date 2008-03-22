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
#include <qstringlist.h>
#include <kapplication.h>
#include <klocale.h>
#include <alsa/asoundlib.h>
#include "sequencerclient.h"
#include "debugdef.h"

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
	m_alsa(true),
	m_showClientNames(false),
	m_translateSysex(false),
	m_needsRefresh(true)
{
    int err;
    snd_seq_port_info_t *pinfo;
    
    err = snd_seq_open( &m_handle, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK );
    checkAlsaError(err, "Sequencer open");
	if (err < 0) {
		QString errorstr = i18n("Fatal error opening the ALSA sequencer. Function: snd_seq_open().\n"
		                   "This usually happens when the kernel doesn't have ALSA support, "
		                   "or the device node (/dev/snd/seq) doesn't exists, "
				           "or the kernel module (snd_seq) is not loaded.\n"
				           "Please check your ALSA/MIDI configuration. Returned error was: %1 (%2)").arg(err).arg(snd_strerror(err));
		throw new std::runtime_error(errorstr.data());
	}
    
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

MidiEvent *SequencerClient::build_translated_sysex(snd_seq_event_t *ev)
{
    int val, cmd;
    unsigned int i, len;
    unsigned char *ptr = (unsigned char *)(ev->data.ext.ptr);
    if (ev->data.ext.len < 6) return NULL;
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
                        if (ev->data.ext.len < 15) return NULL;
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
                if (ev->data.ext.len < 7) return NULL;
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
                        if (ev->data.ext.len < 10) return NULL;
                        data2 = i18n("Full Frame: %1 %2 %3 %4")
                                .arg(ptr[0]).arg(ptr[1]).arg(ptr[2]).arg(ptr[3]);
                        break;
                    case 0x02:
                        if (ev->data.ext.len < 15) return NULL;
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
                        if (ev->data.ext.len < 8) return NULL;
                        val = *ptr++;
                        val += (*ptr++ * 128);
                        data2 = i18n("Bar Marker: %1").arg(val - 8192);
                        break;
                    case 0x02:
                    case 0x42:
                        if (ev->data.ext.len < 9) return NULL;
                        data2 = i18n("Time Signature: %1 (%2/%3)")
                                .arg(ptr[0]).arg(ptr[1]).arg(ptr[2]);
                        break;
                    default:
                        return NULL;
                }
                break;
            case 0x04:
                data1 = i18n("GM Master");
                if (ev->data.ext.len < 8) return NULL;
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
                        if (ev->data.ext.len < (7+len)) return NULL;
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
                        if (ev->data.ext.len < 13) return NULL;
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

MidiEvent *SequencerClient::build_sysex_event(snd_seq_event_t *ev)
{
    if (m_sysex) {
        MidiEvent *me;
        
        if (m_translateSysex) {
            me = build_translated_sysex(ev);
            if (me != NULL) return me;
        }
        
		unsigned int i;
		unsigned char *data = (unsigned char *)ev->data.ext.ptr;
		QString text; 
		for (i = 0; i < ev->data.ext.len; ++i) {
			QString h(QString("%1").arg(data[i], 0, 16));
			if (h.length() < 2) h.prepend('0');
		    h.prepend(' ');
		    text.append(h);
		}
		return new MidiEvent( event_time(ev), 
				      event_source(ev),	
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

QString SequencerClient::client_name(int client_number)
{
    if (showClientNames()) {	
    	if (m_needsRefresh) refreshClientList();
		return m_clients[client_number];
    }
    return QString("%1").arg(client_number);
}

QString SequencerClient::event_source(snd_seq_event_t *ev)
{
    return QString("%1:%2").arg(client_name(ev->source.client))
    			   .arg(ev->source.port);
}

QString SequencerClient::event_addr(snd_seq_event_t *ev)
{
    return QString("%1:%2").arg(client_name(ev->data.addr.client))
    			   .arg(ev->data.addr.port);
}

QString SequencerClient::event_client(snd_seq_event_t *ev)
{
    return client_name(ev->data.addr.client);
}

QString SequencerClient::event_sender(snd_seq_event_t *ev)
{
    return QString("%1:%2").arg(client_name(ev->data.connect.sender.client))
    			   .arg(ev->data.connect.sender.port);
}

QString SequencerClient::event_dest(snd_seq_event_t *ev)
{
    return QString(" %1:%2").arg(client_name(ev->data.connect.dest.client))
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
					event_source(ev),	
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
					event_source(ev),	
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
		return new MidiEvent( event_time(ev), 
				      event_source(ev),	
				      statusText, NULL, param );
    }
    return NULL;
}

MidiEvent *SequencerClient::build_realtime_event( snd_seq_event_t *ev,
						  QString statusText )
{
    if (m_realtime) {
		return new MidiEvent( event_time(ev), event_source(ev), statusText);
    }
    return NULL;
}

MidiEvent *SequencerClient::build_alsa_event( snd_seq_event_t *ev,
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

MidiEvent *SequencerClient::build_controlv_event( snd_seq_event_t *ev, 
						  QString statusText )
{
    if (m_channel) {
		return new MidiEvent( event_time(ev), 
				      event_source(ev),	
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
			me = build_common_event( ev, i18n("MTC Quarter Frame"), common_param(ev) );
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
	        m_needsRefresh = true;
			me = build_alsa_event( ev, i18n("ALSA Port start"), event_addr(ev) );
			break;
	    case SND_SEQ_EVENT_PORT_EXIT:
			me = build_alsa_event( ev, i18n("ALSA Port exit"), event_addr(ev) );
			break;
	    case SND_SEQ_EVENT_PORT_CHANGE:
			m_needsRefresh = true;    
			me = build_alsa_event( ev, i18n("ALSA Port change"), event_addr(ev) );
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
								event_source(ev),	
								QString("Event type %1").arg(ev->type));
    }
    
    return me;
}

QStringList SequencerClient::inputConnections()
{
    return list_ports(SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ);
}

QStringList SequencerClient::outputConnections()
{
    return list_ports(SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE);
}

QStringList SequencerClient::list_ports(unsigned int mask)
{
    QStringList list;
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(m_handle, cinfo) >= 0) {
	int client = snd_seq_client_info_get_client(cinfo);
	if (client == SND_SEQ_CLIENT_SYSTEM || client == m_client)
	    continue;
	snd_seq_port_info_set_client(pinfo, client);
	snd_seq_port_info_set_port(pinfo, -1);
	while (snd_seq_query_next_port(m_handle, pinfo) >= 0) {
	    if ((snd_seq_port_info_get_capability(pinfo) & mask) != mask)
		continue;
	    list += QString("%1:%2").arg(snd_seq_client_info_get_name(cinfo))
				    .arg(snd_seq_port_info_get_port(pinfo));
	}
    }
    return list;
}

void SequencerClient::connect_port(QString name)
{
    snd_seq_addr_t src;
    DEBUGSTREAM << "connecting: " << name << endl;
    checkAlsaError(snd_seq_parse_address(m_handle, &src, name.ascii()),
    		   "snd_seq_parse_address");
    checkAlsaError(snd_seq_connect_from(m_handle, m_input, src.client, src.port),
                   "snd_seq_connect_from");
}

void SequencerClient::disconnect_port(QString name)
{
    snd_seq_addr_t src;
    DEBUGSTREAM << "disconnecting: " << name << endl;
    checkAlsaError(snd_seq_parse_address(m_handle, &src, name.ascii()),
    		   "snd_seq_parse_address");
    checkAlsaError(snd_seq_disconnect_from(m_handle, m_input, src.client, src.port),
    		   "snd_seq_disconnect_from");
}

QStringList SequencerClient::list_subscribers()
{
    QStringList list;
    snd_seq_addr_t root_addr, subs_addr;
    snd_seq_query_subscribe_t *qry;
    snd_seq_client_info_t *cinfo;
    int index = 0;
    
    snd_seq_query_subscribe_alloca(&qry);
    snd_seq_client_info_alloca(&cinfo);
    root_addr.client = m_client;
    root_addr.port = m_input;
    snd_seq_query_subscribe_set_root(qry, &root_addr);
    snd_seq_query_subscribe_set_type(qry,  SND_SEQ_QUERY_SUBS_WRITE);
    snd_seq_query_subscribe_set_index(qry, index);
    while (snd_seq_query_port_subscribers(m_handle, qry) >= 0) {
    	subs_addr = *snd_seq_query_subscribe_get_addr(qry);
    	if (subs_addr.client != SND_SEQ_CLIENT_SYSTEM) {
	    snd_seq_get_any_client_info(m_handle, subs_addr.client, cinfo);
    	    list += QString("%1:%2").arg(snd_seq_client_info_get_name(cinfo))
    	                            .arg(subs_addr.port);
    	}
    	snd_seq_query_subscribe_set_index(qry, ++index);
    }
    return list;
}

void SequencerClient::disconnect_all()
{
    QStringList subs = list_subscribers();
    QStringList::Iterator i;
    for ( i = subs.begin(); i != subs.end(); ++i) {
    	disconnect_port(*i);
    }
}

void SequencerClient::connect_all()
{
    QStringList subs = list_subscribers();
    QStringList ports = inputConnections();
    QStringList::Iterator i;
    for ( i = ports.begin(); i != ports.end(); ++i) {
	if (subs.contains(*i) == 0)
    	    connect_port(*i);
    }
}

void SequencerClient::refreshClientList()
{
    snd_seq_client_info_t *cinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    m_clients.clear();
    //DEBUGSTREAM << "Regenerating client list" << endl;
    while (snd_seq_query_next_client(m_handle, cinfo) >= 0) {
	int cnum = snd_seq_client_info_get_client(cinfo);
        QString cname(snd_seq_client_info_get_name(cinfo));
	m_clients[ cnum ] = cname;
	//DEBUGSTREAM << "client[" << cnum << "] = " << cname << endl;
    }
    m_needsRefresh = false;
}
