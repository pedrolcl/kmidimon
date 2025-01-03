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

#include "proxymodel.h"
#include "sequenceitem.h"
#include "sequencemodel.h"
#include "eventfilter.h"

using namespace drumstick::ALSA;

void ProxyModel::setFilterTrack(int track)
{
    if (track != m_trackFilter) {
        m_trackFilter = track;
        invalidateFilter();
    }
}

void ProxyModel::setFilterChannelMsg(bool newValue)
{
    bool oldValue = m_filter->getFilter(ChannelCategory);
    if (oldValue != newValue) {
        m_filter->setFilter(ChannelCategory, newValue);
        invalidateFilter();
    }
}

void ProxyModel::setFilterCommonMsg(bool newValue)
{
    bool oldValue = m_filter->getFilter(SysCommonCategory);
    if (oldValue != newValue) {
        m_filter->setFilter(SysCommonCategory, newValue);
        invalidateFilter();
    }
}

void ProxyModel::setFilterRealTimeMsg(bool newValue)
{
    bool oldValue = m_filter->getFilter(SysRTCategory);
    if (oldValue != newValue) {
        m_filter->setFilter(SysRTCategory, newValue);
        invalidateFilter();
    }
}

void ProxyModel::setFilterSysexMsg(bool newValue)
{
    bool oldValue = m_filter->getFilter(SysExCategory);
    if (oldValue != newValue) {
        m_filter->setFilter(SysExCategory, newValue);
        invalidateFilter();
    }
}

void ProxyModel::setFilterAlsaMsg(bool newValue)
{
    bool oldValue = m_filter->getFilter(ALSACategory);
    if (oldValue != newValue) {
        m_filter->setFilter(ALSACategory, newValue);
        invalidateFilter();
    }
}

void ProxyModel::setFilterSmfMsg(bool newValue)
{
    bool oldValue = m_filter->getFilter(SMFCategory);
    if (oldValue != newValue) {
        m_filter->setFilter(SMFCategory, newValue);
        invalidateFilter();
    }
}

bool ProxyModel::filterSequencerEvent(const SequencerEvent* ev) const
{
    if (m_filter != nullptr)
        return m_filter->getFilter(ev->getSequencerType());
    return true;
}

bool ProxyModel::filterAcceptsRow(int sourceRow,
        const QModelIndex& /*sourceParent*/) const
{
    bool result = false;
    SequenceModel* sModel = static_cast<SequenceModel*>(sourceModel());
    const SequenceItem* itm = sModel->getItem(sourceRow);
    if (itm != nullptr) {
        const SequencerEvent* ev = itm->getEvent();
        if (ev != nullptr) {
            result = (itm->getTrack() == m_trackFilter) &&
                     filterSequencerEvent(ev);
        }
    }
    return result;
}

bool ProxyModel::showChannelMsg() const
{
    return m_filter->getFilter(ChannelCategory);
}

bool ProxyModel::showCommonMsg() const
{
    return m_filter->getFilter(SysCommonCategory);
}

bool ProxyModel::showRealTimeMsg() const
{
    return m_filter->getFilter(SysRTCategory);
}

bool ProxyModel::showSysexMsg() const
{
    return m_filter->getFilter(SysExCategory);
}

bool ProxyModel::showAlsaMsg() const
{
    return m_filter->getFilter(ALSACategory);
}

bool ProxyModel::showSmfMsg() const
{
    return m_filter->getFilter(SMFCategory);
}
