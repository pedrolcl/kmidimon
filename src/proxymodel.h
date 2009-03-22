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

#ifndef PROXYMODEL_H_
#define PROXYMODEL_H_

#include <QSortFilterProxyModel>
#include <event.h>

using namespace ALSA::Sequencer;

class ProxyModel : public QSortFilterProxyModel
{
public:
    ProxyModel(QObject *parent = 0)
        : QSortFilterProxyModel(parent),
        m_trackFilter(-1),
        m_channelMessageFilter(true),
        m_commonMessageFilter(true),
        m_realtimeMessageFilter(true),
        m_sysexMessageFilter(true),
        m_alsaMessageFilter(true) {}

    virtual ~ProxyModel() {}

    int filterTrack() const { return m_trackFilter; }
    void setFilterTrack(const int track);

    bool showChannelMsg() const { return m_channelMessageFilter; }
    bool showCommonMsg() const { return m_commonMessageFilter; }
    bool showRealTimeMsg() const { return m_realtimeMessageFilter; }
    bool showSysexMsg() const { return m_sysexMessageFilter; }
    bool showAlsaMsg() const { return m_alsaMessageFilter; }
    void setFilterChannelMsg(bool newValue);
    void setFilterCommonMsg(bool newValue);
    void setFilterRealTimeMsg(bool newValue);
    void setFilterSysexMsg(bool newValue);
    void setFilterAlsaMsg(bool newValue);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    bool filterSequencerEvent(const SequencerEvent* ev) const;

    int m_trackFilter;
    bool m_channelMessageFilter;
    bool m_commonMessageFilter;
    bool m_realtimeMessageFilter;
    bool m_sysexMessageFilter;
    bool m_alsaMessageFilter;
};

#endif /* PROXYMODEL_H_ */
