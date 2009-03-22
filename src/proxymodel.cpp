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

#include "proxymodel.h"
#include "sequencemodel.h"

void ProxyModel::setFilterTrack(int track)
{
    if (track != m_trackFilter) {
        m_trackFilter = track;
        invalidateFilter();
    }
}

void ProxyModel::setFilterChannelMsg(bool newValue)
{
    if (m_channelMessageFilter != newValue) {
        m_channelMessageFilter = newValue;
        invalidateFilter();
    }
}

void ProxyModel::setFilterCommonMsg(bool newValue)
{
    if (m_commonMessageFilter != newValue) {
        m_commonMessageFilter = newValue;
        invalidateFilter();
    }
}

void ProxyModel::setFilterRealTimeMsg(bool newValue)
{
    if (m_realtimeMessageFilter != newValue) {
        m_realtimeMessageFilter = newValue;
        invalidateFilter();
    }
}

void ProxyModel::setFilterSysexMsg(bool newValue)
{
    if (m_sysexMessageFilter != newValue) {
        m_sysexMessageFilter = newValue;
        invalidateFilter();
    }
}

void ProxyModel::setFilterAlsaMsg(bool newValue)
{
    if (m_alsaMessageFilter != newValue) {
        m_alsaMessageFilter = newValue;
        invalidateFilter();
    }
}

bool ProxyModel::filterSequencerEvent(const SequencerEvent* ev) const
{
    switch (ev->getSequencerType()) {
        /* MIDI Channel events */
    case SND_SEQ_EVENT_NOTEON:
    case SND_SEQ_EVENT_NOTEOFF:
    case SND_SEQ_EVENT_KEYPRESS:
    case SND_SEQ_EVENT_CONTROLLER:
    case SND_SEQ_EVENT_PGMCHANGE:
    case SND_SEQ_EVENT_CHANPRESS:
    case SND_SEQ_EVENT_PITCHBEND:
    case SND_SEQ_EVENT_CONTROL14:
    case SND_SEQ_EVENT_NONREGPARAM:
    case SND_SEQ_EVENT_REGPARAM:
        return m_channelMessageFilter;

    case SND_SEQ_EVENT_SYSEX:
        return m_sysexMessageFilter;

        /* MIDI Common events */
    case SND_SEQ_EVENT_SONGPOS:
    case SND_SEQ_EVENT_SONGSEL:
    case SND_SEQ_EVENT_QFRAME:
    case SND_SEQ_EVENT_TUNE_REQUEST:
        return m_commonMessageFilter;

        /* MIDI Realtime Events */
    case SND_SEQ_EVENT_START:
    case SND_SEQ_EVENT_CONTINUE:
    case SND_SEQ_EVENT_STOP:
    case SND_SEQ_EVENT_CLOCK:
    case SND_SEQ_EVENT_TICK:
    case SND_SEQ_EVENT_RESET:
    case SND_SEQ_EVENT_SENSING:
        return m_realtimeMessageFilter;

        /* ALSA Client/Port events */
    case SND_SEQ_EVENT_PORT_START:
    case SND_SEQ_EVENT_PORT_EXIT:
    case SND_SEQ_EVENT_PORT_CHANGE:
    case SND_SEQ_EVENT_CLIENT_START:
    case SND_SEQ_EVENT_CLIENT_EXIT:
    case SND_SEQ_EVENT_CLIENT_CHANGE:
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
        return m_alsaMessageFilter;

        /* Other events */
    default:
        return false;
    }
    return false;
}

bool ProxyModel::filterAcceptsRow(int sourceRow,
        const QModelIndex& /*sourceParent*/) const
{
    SequenceModel* sModel = static_cast<SequenceModel*>(sourceModel());
    const SequencerEvent* ev = sModel->getEvent(sourceRow);
    if (ev)
        return (ev->getTag() == m_trackFilter) && filterSequencerEvent(ev);
    return false;
}
