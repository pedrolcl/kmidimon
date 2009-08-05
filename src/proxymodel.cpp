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

#include "eventfilters.h"
#include "proxymodel.h"
#include "sequenceitem.h"
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
    bool channelMessageFilter = g_filters.getFilter(ChannelCategory);
    if (channelMessageFilter != newValue) {
        g_filters.setFilter(ChannelCategory, newValue);
        invalidateFilter();
    }
}

void ProxyModel::setFilterCommonMsg(bool newValue)
{
    bool commonMessageFilter = g_filters.getFilter(SysCommonCategory);
    if (commonMessageFilter != newValue) {
        g_filters.setFilter(SysCommonCategory, newValue);
        invalidateFilter();
    }
}

void ProxyModel::setFilterRealTimeMsg(bool newValue)
{
    bool realtimeMessageFilter = g_filters.getFilter(SysRTCategory);
    if (realtimeMessageFilter != newValue) {
        g_filters.setFilter(SysRTCategory, newValue);
        invalidateFilter();
    }
}

void ProxyModel::setFilterSysexMsg(bool newValue)
{
    bool sysexMessageFilter = g_filters.getFilter(SysExCategory);
    if (sysexMessageFilter != newValue) {
        g_filters.setFilter(SysExCategory, newValue);
        invalidateFilter();
    }
}

void ProxyModel::setFilterAlsaMsg(bool newValue)
{
    bool alsaMessageFilter = g_filters.getFilter(ALSACategory);
    if (alsaMessageFilter != newValue) {
        g_filters.setFilter(ALSACategory, newValue);
        invalidateFilter();
    }
}

void ProxyModel::setFilterSmfMsg(bool newValue)
{
    bool smfMessageFilter = g_filters.getFilter(SMFCategory);
    if (smfMessageFilter != newValue) {
        g_filters.setFilter(SMFCategory, newValue);
        invalidateFilter();
    }
}

bool ProxyModel::filterSequencerEvent(const SequencerEvent* ev) const
{
/*
    switch (ev->getSequencerType()) {
    // MIDI Channel events
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

    // MIDI Common events
    case SND_SEQ_EVENT_SONGPOS:
    case SND_SEQ_EVENT_SONGSEL:
    case SND_SEQ_EVENT_QFRAME:
    case SND_SEQ_EVENT_TUNE_REQUEST:
        return m_commonMessageFilter;

    // MIDI Realtime Events
    case SND_SEQ_EVENT_START:
    case SND_SEQ_EVENT_CONTINUE:
    case SND_SEQ_EVENT_STOP:
    case SND_SEQ_EVENT_CLOCK:
    case SND_SEQ_EVENT_TICK:
    case SND_SEQ_EVENT_RESET:
    case SND_SEQ_EVENT_SENSING:
        return m_realtimeMessageFilter;

    // ALSA Client/Port events
    case SND_SEQ_EVENT_PORT_START:
    case SND_SEQ_EVENT_PORT_EXIT:
    case SND_SEQ_EVENT_PORT_CHANGE:
    case SND_SEQ_EVENT_CLIENT_START:
    case SND_SEQ_EVENT_CLIENT_EXIT:
    case SND_SEQ_EVENT_CLIENT_CHANGE:
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
        return m_alsaMessageFilter;

    // SMF Meta events
    case SND_SEQ_EVENT_USR_VAR0:
    case SND_SEQ_EVENT_USR_VAR1:
    case SND_SEQ_EVENT_USR_VAR2:
    case SND_SEQ_EVENT_USR_VAR3:
    case SND_SEQ_EVENT_USR_VAR4:
    case SND_SEQ_EVENT_USR0:
    case SND_SEQ_EVENT_USR1:
    case SND_SEQ_EVENT_USR2:
    case SND_SEQ_EVENT_USR3:
    case SND_SEQ_EVENT_USR4:
    case SND_SEQ_EVENT_USR5:
    case SND_SEQ_EVENT_USR6:
    case SND_SEQ_EVENT_USR7:
    case SND_SEQ_EVENT_USR8:
    case SND_SEQ_EVENT_USR9:
    case SND_SEQ_EVENT_KEYSIGN:
    case SND_SEQ_EVENT_TIMESIGN:
    case SND_SEQ_EVENT_TEMPO:
        return m_smfMessageFilter;
    // Other events
    default:
        return true;
    }
*/
    if (g_filters.contains(ev->getSequencerType()))
        return g_filters.getFilter(ev->getSequencerType());
    return true;
}

bool ProxyModel::filterAcceptsRow(int sourceRow,
        const QModelIndex& /*sourceParent*/) const
{
    SequenceModel* sModel = static_cast<SequenceModel*>(sourceModel());
    const SequenceItem* itm = sModel->getItem(sourceRow);
    if (itm) {
        const SequencerEvent* ev = itm->getEvent();
        if (ev) return (itm->getTrack() == m_trackFilter) &&
                       filterSequencerEvent(ev);
    }
    return false;
}

bool ProxyModel::showChannelMsg() const
{
    return g_filters.getFilter(ChannelCategory);
}

bool ProxyModel::showCommonMsg() const
{
    return g_filters.getFilter(SysCommonCategory);
}

bool ProxyModel::showRealTimeMsg() const
{
    return g_filters.getFilter(SysRTCategory);
}

bool ProxyModel::showSysexMsg() const
{
    return g_filters.getFilter(SysExCategory);
}

bool ProxyModel::showAlsaMsg() const
{
    return g_filters.getFilter(ALSACategory);
}

bool ProxyModel::showSmfMsg() const
{
    return g_filters.getFilter(SMFCategory);
}
