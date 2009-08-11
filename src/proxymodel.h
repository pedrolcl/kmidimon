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
#include <alsaevent.h>

class EventFilter;

USE_ALSASEQ_NAMESPACE

class ProxyModel : public QSortFilterProxyModel
{
public:
    ProxyModel(QObject *parent = 0)
        : QSortFilterProxyModel(parent),
        m_trackFilter(-1),
        m_filter(NULL)
    {}
    virtual ~ProxyModel() {}

    int filterTrack() const { return m_trackFilter; }
    void setFilterTrack(const int track);

    bool showChannelMsg() const;
    bool showCommonMsg() const;
    bool showRealTimeMsg() const;
    bool showSysexMsg() const;
    bool showAlsaMsg() const;
    bool showSmfMsg() const;
    void setFilterChannelMsg(bool newValue);
    void setFilterCommonMsg(bool newValue);
    void setFilterRealTimeMsg(bool newValue);
    void setFilterSysexMsg(bool newValue);
    void setFilterAlsaMsg(bool newValue);
    void setFilterSmfMsg(bool newValue);
    void setFilter(EventFilter* value) { m_filter = value; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    bool filterSequencerEvent(const SequencerEvent* ev) const;

    int m_trackFilter;
    EventFilter* m_filter;
};

#endif /* PROXYMODEL_H_ */
