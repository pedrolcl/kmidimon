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

#ifndef SEQUENCEMODEL_H_
#define SEQUENCEMODEL_H_

#include <QAbstractItemModel>
#include <QMap>
#include <event.h>
#include "sequenceitem.h"

using namespace ALSA::Sequencer;

typedef QMap<int,QString> ClientsMap;

class SequenceModel : public QAbstractItemModel
{
public:
    SequenceModel(QObject* parent = 0) :
        QAbstractItemModel(parent),
        m_channelMessageFilter(true),
        m_commonMessageFilter(true),
        m_realtimeMessageFilter(true),
        m_sysexMessageFilter(true),
        m_alsaMessageFilter(true),
        m_showClientNames(false),
        m_translateSysex(false)
        { }

    virtual ~SequenceModel();

    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;

    void setSorted(bool s) { m_sorted = s; }
    bool isSorted() { return m_sorted; }
    void addItem(SequenceItem& itm);
    void clear();
    void saveToStream(QTextStream& str);

    bool showChannelMsg() const { return m_channelMessageFilter; }
    bool showCommonMsg() const { return m_commonMessageFilter; }
    bool showRealTimeMsg() const { return m_realtimeMessageFilter; }
    bool showSysexMsg() const { return m_sysexMessageFilter; }
    bool showAlsaMsg() const { return m_alsaMessageFilter; }
    bool showClientNames() const { return m_showClientNames; }
    bool translateSysex() const { return m_translateSysex; }
    void setFilterChannelMsg(bool newValue) { m_channelMessageFilter = newValue; }
    void setFilterCommonMsg(bool newValue) { m_commonMessageFilter = newValue; }
    void setFilterRealTimeMsg(bool newValue) { m_realtimeMessageFilter = newValue; }
    void setFilterSysexMsg(bool newValue) { m_sysexMessageFilter = newValue; }
    void setFilterAlsaMsg(bool newValue) { m_alsaMessageFilter = newValue; }
    void setShowClientNames(bool newValue) { m_showClientNames = newValue; }
    void setTranslateSysex(bool newValue) { m_translateSysex = newValue; }
    void updateClients(ClientsMap& newmap) { m_clients = newmap; }

private:
    bool filterSequencerEvent(SequencerEvent* ev) const;

    QString client_name(int client_number) const;
    QString event_time(SequenceItem& itm) const;
    QString event_source(SequencerEvent *ev) const;
    QString event_ticks(SequencerEvent *ev) const;
    QString event_client(SequencerEvent *ev) const;
    QString event_addr(SequencerEvent *ev) const;
    QString event_sender(SequencerEvent *ev) const;
    QString event_dest(SequencerEvent *ev) const;
    QString common_param(SequencerEvent *ev) const;
    QString event_kind(SequencerEvent *ev) const;
    QString event_channel(SequencerEvent *ev) const;
    QString event_data1(SequencerEvent *ev) const;
    QString event_data2(SequencerEvent *ev) const;
    QString note_key(SequencerEvent* ev) const;
    QString note_velocity(SequencerEvent* ev) const;
    QString control_param(SequencerEvent* ev) const;
    QString control_value(SequencerEvent* ev) const;
    QString program_number(SequencerEvent* ev) const;
    QString pitchbend_value(SequencerEvent* ev) const;
    QString sysex_type(SequencerEvent *ev) const;
    QString sysex_chan(SequencerEvent *ev) const;
    QString sysex_data1(SequencerEvent *ev) const;
    QString sysex_data2(SequencerEvent *ev) const;
    QString sysex_mtc_setup(int id) const;
    QString sysex_mtc(int id, int length, unsigned char *ptr) const;
    QString sysex_mmc(int id, int length, unsigned char *ptr) const;

    bool m_channelMessageFilter;
    bool m_commonMessageFilter;
    bool m_realtimeMessageFilter;
    bool m_sysexMessageFilter;
    bool m_alsaMessageFilter;
    bool m_showClientNames;
    bool m_translateSysex;
    bool m_sorted;

    ClientsMap m_clients;
    QList<SequenceItem> m_items;
};

#endif /* SEQUENCEMODEL_H_ */
