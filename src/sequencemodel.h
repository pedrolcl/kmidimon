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

#ifndef SEQUENCEMODEL_H
#define SEQUENCEMODEL_H

#include <QAbstractItemModel>
#include <QMap>

#include <alsaevent.h>
#include <qsmf.h>

#include "sequenceitem.h"
#include "instrument.h"

using namespace aseqmm;

class EventFilter;

typedef QMap<int,QString> ClientsMap;

class Song : public QList<SequenceItem>
{
public:
    virtual ~Song() {}
    void sort();
};

typedef QListIterator<SequenceItem> SongIterator;

class SequenceModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    SequenceModel(QObject* parent = 0);
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

    void setCurrentRow(const int row);
    QModelIndex getCurrentRow();
    QModelIndex getRowIndex(int row) { return createIndex(row, 0); }

    bool isEmpty() { return m_items.isEmpty(); }
    void addItem(SequenceItem& itm);
    const SequenceItem* getItem(const int row) const;
    const SequencerEvent* getEvent(const int row) const;
    void clear();
    void saveToTextStream(QTextStream& str);
    void saveToFile(const QString& path);
    void loadFromFile(const QString& path);
    void appendEvent(SequencerEvent* ev);

    bool showClientNames() const { return m_showClientNames; }
    bool translateSysex() const { return m_translateSysex; }
    bool translateNotes() const { return m_translateNotes; }
    bool translateCtrls() const { return m_translateCtrls; }
    bool useFlats() const { return m_useFlats; }
    void setShowClientNames(bool newValue) { m_showClientNames = newValue; }
    void setTranslateSysex(bool newValue) { m_translateSysex = newValue; }
    void setTranslateNotes(bool newValue) { m_translateNotes = newValue; }
    void setTranslateCtrls(bool newValue) { m_translateCtrls = newValue; }
    void setUseFlats(bool newValue) { m_useFlats = newValue; }

    QString getInstrumentName() const { return m_instrumentName; }
    void setInstrumentName(const QString name);

    void updateClients(ClientsMap& newmap) { m_clients = newmap; }
    void updateQueue(const int q) { m_queueId = q; }
    void updatePort(const int p) { m_portId = p; }

    int currentTrack() const { return m_currentTrack; }
    void setCurrentTrack(int t) { m_currentTrack = t; }

    int getSMFFormat() const { return m_format; }
    int getSMFTracks() const { return m_ntrks; }
    int getSMFDivision() const { return m_division; }
    int getInitialTempo() const { return m_initialTempo; }
    void setInitialTempo(int tempo) { m_initialTempo = tempo; }
    void setDivision(int division) { m_division = division; }
    void sortSong() { m_items.sort(); }
    Song* getSong() { return &m_items; }
    const SequenceItem& lastItem() const { return m_items.last(); }
    QStringList getInstruments() const;
    void setFilter(EventFilter* value) { m_filter = value; }
    QString getDuration() const;

    void setEncoding(const QString& encoding);
    QString getEncoding() const { return m_encoding; }

public slots:
    void headerEvent(int format, int ntrks, int division);
    void trackStartEvent();
    void trackEndEvent();
    void endOfTrackEvent();
    void noteOnEvent(int chan, int pitch, int vol);
    void noteOffEvent(int chan, int pitch, int vol);
    void keyPressEvent(int chan, int pitch, int press);
    void ctlChangeEvent(int chan, int ctl, int value);
    void pitchBendEvent(int chan, int value);
    void programEvent(int chan, int patch);
    void chanPressEvent(int chan, int press);
    void sysexEvent(const QByteArray& data);
    void variableEvent(const QByteArray& data);
    void metaMiscEvent(int typ, const QByteArray& data);
    void textEvent(int type, const QString& data);
    void tempoEvent(int tempo);
    void timeSigEvent(int b0, int b1, int b2, int b3);
    void keySigEvent(int b0, int b1);
    void errorHandler(const QString& errorStr);
    void trackHandler(int track);
    void seqNum(int seq);
    void forcedChannel(int channel);
    void forcedPort(int port);
    void smpteEvent(int b0, int b1, int b2, int b3, int b4);

signals:
    void loadProgress(int);

private:
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
    QString note_name(const int note) const;
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
    QString tempo_bpm(const SequencerEvent *ev) const;
    QString tempo_npt(const SequencerEvent *ev) const;
    QString text_type(const SequencerEvent *ev) const;
    QString text_data(const SequencerEvent *ev) const;
    QString time_sig(const SequencerEvent *ev) const;
    QString key_sig(const SequencerEvent *ev) const;
    QString smpte(const SequencerEvent *ev) const;

    bool m_showClientNames;
    bool m_translateSysex;
    bool m_translateNotes;
    bool m_translateCtrls;
    bool m_useFlats;
    int m_currentTrack;
    int m_currentRow;
    int m_portId;
    int m_queueId;

    int m_format;
    int m_ntrks;
    int m_division;
    int m_initialTempo;
    int m_lastBank[16];
    int m_lastPatch[16];
    int m_lastCtlMSB;
    int m_lastCtlLSB;

    double m_duration;

    ClientsMap m_clients;
    InstrumentList m_insList;
    QString m_instrumentName;
    QString m_encoding;
    Song m_items;
    Song m_loadedSong;
    QSmf* m_smf;
    Instrument* m_ins;
    Instrument* m_ins2;
    EventFilter* m_filter;
};

#endif /* SEQUENCEMODEL_H */
