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

    void setOrdered(bool s) { m_ordered = s; }
    bool isOrdered() { return m_ordered; }
    void addItem(SequenceItem& itm);
    const SequencerEvent* getEvent(const int row) const;
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
    bool filterSequencerEvent(const SequencerEvent* ev) const;

    QString client_name(const int client_number) const;
    QString event_time(const SequenceItem& itm) const;
    QString event_source(const SequencerEvent *ev) const;
    QString event_ticks(const SequencerEvent *ev) const;
    QString event_client(const SequencerEvent *ev) const;
    QString event_addr(const SequencerEvent *ev) const;
    QString event_sender(const SequencerEvent *ev) const;
    QString event_dest(const SequencerEvent *ev) const;
    QString common_param(const SequencerEvent *ev) const;
    QString event_kind(const SequencerEvent *ev) const;
    QString event_channel(const SequencerEvent *ev) const;
    QString event_data1(const SequencerEvent *ev) const;
    QString event_data2(const SequencerEvent *ev) const;
    QString note_key(const SequencerEvent* ev) const;
    QString note_velocity(const SequencerEvent* ev) const;
    QString control_param(const SequencerEvent* ev) const;
    QString control_value(const SequencerEvent* ev) const;
    QString program_number(const SequencerEvent* ev) const;
    QString pitchbend_value(const SequencerEvent* ev) const;
    QString chanpress_value(const SequencerEvent* ev) const;
    QString sysex_type(const SequencerEvent *ev) const;
    QString sysex_chan(const SequencerEvent *ev) const;
    QString sysex_data1(const SequencerEvent *ev) const;
    QString sysex_data2(const SequencerEvent *ev) const;
    QString sysex_mtc_setup(const int id) const;
    QString sysex_mtc(int id, int length, unsigned char *ptr) const;
    QString sysex_mmc(int id, int length, unsigned char *ptr) const;

    bool m_channelMessageFilter;
    bool m_commonMessageFilter;
    bool m_realtimeMessageFilter;
    bool m_sysexMessageFilter;
    bool m_alsaMessageFilter;
    bool m_showClientNames;
    bool m_translateSysex;
    bool m_ordered;

    ClientsMap m_clients;
    QList<SequenceItem> m_items;
};

#endif /* SEQUENCEMODEL_H_ */
