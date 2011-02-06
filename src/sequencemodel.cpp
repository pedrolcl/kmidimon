/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2011 Pedro Lopez-Cabanillas                        *
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

#include "sequencemodel.h"
#include "sequenceradaptor.h"
#include "kmidimon.h"
#include "eventfilter.h"
#include <cmath>
#include <KDE/KLocale>
#include <KDE/KApplication>
#include <KDE/KStandardDirs>
#include <KDE/KCharsets>
#include <KDE/KMimeType>
#include <KDE/KDebug>
#include <QtCore/QFile>
#include <QtCore/QDataStream>
#include <QtCore/QListIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QTime>
#include <QtCore/QTextCodec>

static inline bool eventLessThan(const SequenceItem& s1, const SequenceItem& s2)
{
    return s1.getTicks() < s2.getTicks();
}

void Song::sort()
{
    qStableSort(begin(), end(), eventLessThan);
}

void Song::clear()
{
    QList<SequenceItem>::clear();
    m_mutedState.clear();
    m_last = 0;
}

void Song::setLast(long last)
{
    if (last > m_last)
        m_last = last;
}

void Song::setMutedState(int track, bool muted)
{
    if (muted != m_mutedState[track]) {
        m_mutedState[track] = muted;
    }
}

SequenceModel::SequenceModel(QObject* parent) :
        QAbstractItemModel(parent),
        m_showClientNames(false),
        m_translateSysex(false),
        m_translateNotes(false),
        m_translateCtrls(false),
        m_useFlats(false),
        m_reportsFilePos(false),
        m_currentTrack(0),
        m_currentRow(0),
        m_portId(0),
        m_queueId(0),
        m_format(0),
        m_ntrks(1),
        m_division(RESOLUTION),
        m_initialTempo(TEMPO_BPM),
        m_duration(0),
        m_ins(0),
        m_ins2(0),
        m_filter(0),
        m_appendFunc(0)
{
    m_smf = new QSmf(this);
    connect(m_smf, SIGNAL(signalSMFHeader(int,int,int)),
                   SLOT(headerEvent(int,int,int)));
    connect(m_smf, SIGNAL(signalSMFTrackStart()),
                   SLOT(trackStartEvent()));
    connect(m_smf, SIGNAL(signalSMFTrackEnd()),
                   SLOT(trackEndEvent()));
    connect(m_smf, SIGNAL(signalSMFNoteOn(int,int,int)),
                   SLOT(noteOnEvent(int,int,int)));
    connect(m_smf, SIGNAL(signalSMFNoteOff(int,int,int)),
                   SLOT(noteOffEvent(int,int,int)));
    connect(m_smf, SIGNAL(signalSMFKeyPress(int,int,int)),
                   SLOT(keyPressEvent(int,int,int)));
    connect(m_smf, SIGNAL(signalSMFCtlChange(int,int,int)),
                   SLOT(smfCtlChangeEvent(int,int,int)));
    connect(m_smf, SIGNAL(signalSMFPitchBend(int,int)),
                   SLOT(pitchBendEvent(int,int)));
    connect(m_smf, SIGNAL(signalSMFProgram(int,int)),
                   SLOT(programEvent(int,int)));
    connect(m_smf, SIGNAL(signalSMFChanPress(int,int)),
                   SLOT(chanPressEvent(int,int)));
    connect(m_smf, SIGNAL(signalSMFSysex(const QByteArray&)),
                   SLOT(sysexEvent(const QByteArray&)));
    connect(m_smf, SIGNAL(signalSMFMetaUnregistered(int, const QByteArray&)),
                   SLOT(metaMiscEvent(int, const QByteArray&)));
    connect(m_smf, SIGNAL(signalSMFSeqSpecific(const QByteArray&)),
                   SLOT(seqSpecificEvent(const QByteArray&)));
    connect(m_smf, SIGNAL(signalSMFText(int,const QString&)),
                   SLOT(textEvent(int,const QString&)));
    connect(m_smf, SIGNAL(signalSMFendOfTrack()),
                   SLOT(endOfTrackEvent()));
    connect(m_smf, SIGNAL(signalSMFTempo(int)),
                   SLOT(tempoEvent(int)));
    connect(m_smf, SIGNAL(signalSMFTimeSig(int,int,int,int)),
                   SLOT(timeSigEvent(int,int,int,int)));
    connect(m_smf, SIGNAL(signalSMFKeySig(int,int)),
                   SLOT(keySigEventSMF(int,int)));
    connect(m_smf, SIGNAL(signalSMFError(const QString&)),
                   SLOT(errorHandlerSMF(const QString&)));
    connect(m_smf, SIGNAL(signalSMFWriteTrack(int)),
                   SLOT(trackHandler(int)));
    connect(m_smf, SIGNAL(signalSMFSequenceNum(int)),
                   SLOT(seqNum(int)));
    connect(m_smf, SIGNAL(signalSMFforcedChannel(int)),
                   SLOT(forcedChannel(int)));
    connect(m_smf, SIGNAL(signalSMFforcedPort(int)),
                   SLOT(forcedPort(int)));
    connect(m_smf, SIGNAL(signalSMFSmpte(int,int,int,int,int)),
                   SLOT(smpteEvent(int,int,int,int,int)));

    m_wrk = new QWrk(this);
    connect(m_wrk, SIGNAL(signalWRKError(const QString&)),
                   SLOT(errorHandlerWRK(const QString&)));
    connect(m_wrk, SIGNAL(signalWRKUnknownChunk(int,const QByteArray&)),
                   SLOT(unknownChunk(int,const QByteArray&)));
    connect(m_wrk, SIGNAL(signalWRKHeader(int,int)),
                   SLOT(fileHeader(int,int)));
    connect(m_wrk, SIGNAL(signalWRKEnd()),
                   SLOT(endOfWrk()));
    connect(m_wrk, SIGNAL(signalWRKStreamEnd(long)),
                   SLOT(streamEndEvent(long)));
    connect(m_wrk, SIGNAL(signalWRKGlobalVars()),
                   SLOT(globalVars()));
    connect(m_wrk, SIGNAL(signalWRKTrack(const QString&, const QString&, int,int,int,int,int,bool,bool,bool)),
                   SLOT(trackHeader(const QString&, const QString&, int,int,int,int,int,bool,bool,bool)));
    connect(m_wrk, SIGNAL(signalWRKTimeBase(int)),
                   SLOT(timeBase(int)));
    connect(m_wrk, SIGNAL(signalWRKNote(int,long,int,int,int,int)),
                   SLOT(noteEvent(int,long,int,int,int,int)));
    connect(m_wrk, SIGNAL(signalWRKKeyPress(int,long,int,int,int)),
                   SLOT(keyPressEvent(int,long,int,int,int)));
    connect(m_wrk, SIGNAL(signalWRKCtlChange(int,long,int,int,int)),
                   SLOT(wrkCtlChangeEvent(int,long,int,int,int)));
    connect(m_wrk, SIGNAL(signalWRKPitchBend(int,long,int,int)),
                   SLOT(pitchBendEvent(int,long,int,int)));
    connect(m_wrk, SIGNAL(signalWRKProgram(int,long,int,int)),
                   SLOT(programEvent(int,long,int,int)));
    connect(m_wrk, SIGNAL(signalWRKChanPress(int,long,int,int)),
                   SLOT(chanPressEvent(int,long,int,int)));
    connect(m_wrk, SIGNAL(signalWRKSysexEvent(int,long,int)),
                   SLOT(sysexEvent(int,long,int)));
    connect(m_wrk, SIGNAL(signalWRKSysex(int,const QString&,bool,int,const QByteArray&)),
                   SLOT(sysexEventBank(int,const QString&,bool,int,const QByteArray&)));
    connect(m_wrk, SIGNAL(signalWRKText(int,long,int,const QString&)),
                   SLOT(textEvent(int,long,int,const QString&)));
    connect(m_wrk, SIGNAL(signalWRKTimeSig(int,int,int)),
                   SLOT(timeSigEvent(int,int,int)));
    connect(m_wrk, SIGNAL(signalWRKKeySig(int,int)),
                   SLOT(keySigEventWRK(int,int)));
    connect(m_wrk, SIGNAL(signalWRKTempo(long,int)),
                   SLOT(tempoEvent(long,int)));
    connect(m_wrk, SIGNAL(signalWRKTrackPatch(int,int)),
                   SLOT(trackPatch(int,int)));
    connect(m_wrk, SIGNAL(signalWRKComments(const QString&)),
                   SLOT(comments(const QString&)));
    connect(m_wrk, SIGNAL(signalWRKVariableRecord(const QString&,const QByteArray&)),
                   SLOT(variableRecord(const QString&,const QByteArray&)));
    connect(m_wrk, SIGNAL(signalWRKNewTrack(const QString&,int,int,int,int,int,bool,bool,bool)),
                   SLOT(newTrackHeader(const QString&,int,int,int,int,int,bool,bool,bool)));
    connect(m_wrk, SIGNAL(signalWRKTrackName(int,const QString&)),
                   SLOT(trackName(int,const QString&)));
    connect(m_wrk, SIGNAL(signalWRKTrackVol(int,int)),
                   SLOT(trackVol(int,int)));
    connect(m_wrk, SIGNAL(signalWRKTrackBank(int,int)),
                   SLOT(trackBank(int,int)));
    connect(m_wrk, SIGNAL(signalWRKSegment(int,long,const QString&)),
                   SLOT(segment(int,long,const QString&)));
    connect(m_wrk, SIGNAL(signalWRKChord(int,long,const QString&,const QByteArray&)),
                   SLOT(chord(int,long,const QString&,const QByteArray&)));
    connect(m_wrk, SIGNAL(signalWRKExpression(int,long,int,const QString&)),
                   SLOT(expression(int,long,int,const QString&)));

    m_ove = new QOve(this);
    connect(m_ove, SIGNAL(signalOVEError(const QString&)),
                   SLOT(oveErrorHandler(const QString&)));
    connect(m_ove, SIGNAL(signalOVEHeader(int,int)),
                   SLOT(oveFileHeader(int,int)));
    connect(m_ove, SIGNAL(signalOVEEnd()),
                   SLOT(endOfWrk()));
    connect(m_ove, SIGNAL(signalOVENoteOn(int, long, int, int, int)),
                   SLOT(oveNoteOnEvent(int, long, int, int, int)));
    connect(m_ove, SIGNAL(signalOVENoteOff(int, long, int, int, int)),
                   SLOT(oveNoteOffEvent(int, long, int, int, int)));
    connect(m_ove, SIGNAL(signalOVEKeyPress(int,long,int,int,int)),
                   SLOT(keyPressEvent(int,long,int,int,int)));
    connect(m_ove, SIGNAL(signalOVECtlChange(int,long,int,int,int)),
                   SLOT(wrkCtlChangeEvent(int,long,int,int,int)));
    connect(m_ove, SIGNAL(signalOVEPitchBend(int,long,int,int)),
                   SLOT(pitchBendEvent(int,long,int,int)));
    connect(m_ove, SIGNAL(signalOVEProgram(int,long,int,int)),
                   SLOT(programEvent(int,long,int,int)));
    connect(m_ove, SIGNAL(signalOVEChanPress(int,long,int,int)),
                   SLOT(chanPressEvent(int,long,int,int)));
    connect(m_ove, SIGNAL(signalOVESysexEvent(int,long,int)),
                   SLOT(sysexEvent(int,long,int)));
    connect(m_ove, SIGNAL(signalOVESysex(int,const QString&,bool,int,const QByteArray&)),
                   SLOT(sysexEventBank(int,const QString&,bool,int,const QByteArray&)));
    connect(m_ove, SIGNAL(signalOVETempo(long,int)),
                   SLOT(tempoEvent(long,int)));
    connect(m_ove, SIGNAL(signalOVETrackPatch(int,int,int)),
                   SLOT(oveTrackPatch(int,int,int)));
    connect(m_ove, SIGNAL(signalOVENewTrack(const QString&,int,int,int,int,int,bool,bool,bool)),
                   SLOT(newTrackHeader(const QString&,int,int,int,int,int,bool,bool,bool)));
    connect(m_ove, SIGNAL(signalOVETrackVol(int,int,int)),
                   SLOT(oveTrackVol(int,int,int)));
    connect(m_ove, SIGNAL(signalOVETrackBank(int,int,int)),
                   SLOT(oveTrackBank(int,int,int)));

    connect(m_ove, SIGNAL(signalOVEText(int,long,const QString&)),
                   SLOT(oveTextEvent(int,long,const QString&)));
    connect(m_ove, SIGNAL(signalOVETimeSig(int,long,int,int)),
                   SLOT(oveTimeSigEvent(int,long,int,int)));
    connect(m_ove, SIGNAL(signalOVEKeySig(int,long,int)),
                   SLOT(oveKeySigEvent(int,long,int)));

    connect(m_ove, SIGNAL(signalOVEChord(int,long,const QString&,const QByteArray&)),
                   SLOT(chord(int,long,const QString&,const QByteArray&)));
    connect(m_ove, SIGNAL(signalOVEExpression(int,long,int,const QString&)),
                   SLOT(expression(int,long,int,const QString&)));


    QString stdins =  KStandardDirs::locate("appdata", "standards.ins");
    if (!stdins.isEmpty())
    	m_insList.load(stdins);
}

SequenceModel::~SequenceModel()
{
    clear();
}

Qt::ItemFlags
SequenceModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant
SequenceModel::headerData(int section, Qt::Orientation orientation,
                          int role) const
{
    if ((orientation == Qt::Horizontal) && (role == Qt::DisplayRole)) {
        switch(section) {
        case 0:
            return i18n("Ticks");
        case 1:
            return i18n("Time");
        case 2:
            return i18nc("event origin","Source");
        case 3:
            return i18n("Event kind");
        case 4:
            return i18n("Chan");
        case 5:
            return i18n("Data 1");
        case 6:
            return i18n("Data 2");
        case 7:
            return i18n("Data 3");
        }
    }
    return QVariant();
}

QModelIndex
SequenceModel::index(int row, int column,
                     const QModelIndex& /*parent*/) const
{
    if ((row < m_items.count()) && (column < COLUMN_COUNT))
        return createIndex( row, column );
    return QModelIndex();
}

QModelIndex
SequenceModel::parent(const QModelIndex& /*index*/) const
{
    return QModelIndex();
}

void
SequenceModel::setCurrentRow(const int row)
{
    m_currentRow = row;
}

QModelIndex
SequenceModel::getCurrentRow()
{
    return createIndex(m_currentRow , 0);
}

int
SequenceModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.count();
}

int
SequenceModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return COLUMN_COUNT;
}

QVariant
SequenceModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        if ( role == Qt::DisplayRole ) {
            SequenceItem itm = m_items[index.row()];
            const SequencerEvent* ev = itm.getEvent();
            if (ev != NULL) {
                switch (index.column()) {
                case 0:
                    return event_ticks(ev);
                case 1:
                    return event_time(itm);
                case 2:
                    return event_source(ev);
                case 3:
                    return event_kind(ev);
                case 4:
                    return event_channel(ev);
                case 5:
                    return event_data1(ev);
                case 6:
                    return event_data2(ev);
                case 7:
                    return event_data3(ev);
                }
            }
        } else
        if ( role == Qt::TextAlignmentRole ) {
            switch(index.column()) {
            case 0:
                return Qt::AlignRight;
            case 1:
                return Qt::AlignRight;
            case 2:
                return Qt::AlignRight;
            case 3:
                return Qt::AlignLeft;
            case 4:
                return Qt::AlignRight;
            case 5:
                return Qt::AlignRight;
            case 6:
                return Qt::AlignRight;
            case 7:
                return Qt::AlignLeft;
            }
        }
    }
    return QVariant();
}

void
SequenceModel::addItem(SequenceItem& itm)
{
    int where = m_items.count();
    itm.setTrack(m_currentTrack);
    beginInsertRows(QModelIndex(), where, where);
    m_items.append(itm);
    endInsertRows();
}

void
SequenceModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, m_items.count());
    QList<SequenceItem>::Iterator it;
    for ( it = m_items.begin(); it != m_items.end(); ++it )
        (*it).deleteEvent();
    m_items.clear();
    endRemoveRows();
    m_format = 0;
    m_ntrks = 1;
    m_division = RESOLUTION;
    m_initialTempo = TEMPO_BPM;
    m_currentTrack = 0;
    m_currentRow = 0;
    m_duration = 0;
    m_fileFormat.clear();
    m_savedSysexEvents.clear();
    m_trackMap.clear();
    m_bars.clear();
    m_tempos.clear();
    m_loadingMessages.clear();
    m_lastCtlMSB = 0;
    m_lastCtlLSB = 0;
    for (int i=0; i<16; ++i) {
        m_lastBank[i] = 0;
        m_lastPatch[i] = 0;
    }
}

void
SequenceModel::saveToTextStream(QTextStream& str)
{
    foreach ( const SequenceItem& itm, m_items ) {
        const SequencerEvent* ev = itm.getEvent();
        if (ev != NULL) {
            str << event_ticks(ev).trimmed() << ","
                << event_time(itm).trimmed() << ","
                << event_source(ev).trimmed() << ","
                << event_channel(ev).trimmed() << ","
                << event_kind(ev).trimmed() << ","
                << event_data1(ev).trimmed() << ","
                << event_data2(ev).trimmed() << ","
                << event_data3(ev).trimmed() << endl;
        }
    }
}

QString
SequenceModel::sysex_type(const SequencerEvent *ev) const
{
    const SysExEvent *sev = static_cast<const SysExEvent*>(ev);
    if (sev != NULL) {
        if (m_translateSysex) {
            unsigned char *ptr = (unsigned char *) sev->getData();
            if (sev->getLength() < 6) return QString();
            if (ptr[0] != 0xf0) return QString();
            int msgId = ptr[1];
            switch (msgId) {
            case 0x7e:
                return i18n("Universal Non Real Time SysEx");
            case 0x7f:
                return i18n("Universal Real Time SysEx");
            default:
                break;
            }
        }
        return m_filter->getName(ev->getSequencerType());
    }
    return QString();
}

QString
SequenceModel::sysex_chan(const SequencerEvent *ev) const
{
    const SysExEvent *sev = static_cast<const SysExEvent*>(ev);
    if (sev != NULL) {
        if (m_translateSysex) {
            unsigned char *ptr = (unsigned char *) sev->getData();
            if (sev->getLength() < 6) return QString();
            if (ptr[0] != 0xf0) return QString();
            unsigned char deviceId = ptr[2];
            if ( deviceId == 0x7f )
                return i18nc("cast or scattered in all directions","broadcast");
            else
                return i18n("device %1", deviceId);
        }
        return "-";
    }
    return QString();
}

QString
SequenceModel::sysex_data1(const SequencerEvent *ev) const
{
    const SysExEvent *sev = static_cast<const SysExEvent*>(ev);
    if (sev != NULL) {
        if (m_translateSysex) {
            unsigned char *ptr = (unsigned char *) sev->getData();
            if (sev->getLength() < 6) return QString();
            if (ptr[0] != 0xf0) return QString();
            int msgId = ptr[1];
            int subId1 = ptr[3];
            if (msgId == 0x7e) { // Universal Non Real Time
                switch (subId1) {
                    case 0x01:
                    case 0x02:
                    case 0x03:
                        return i18n("Sample Dump");
                    case 0x04:
                        return i18n("MTC");
                    case 0x05:
                        return i18n("Sample Dump");
                    case 0x06:
                        return i18nc("General Info", "Gen.Info");
                    case 0x07:
                        return i18n("File Dump");
                    case 0x08:
                        return i18n("Tuning");
                    case 0x09:
                        return i18nc("General MIDI mode", "GM Mode");
                    case 0x0a:
                        return i18nc("Downloadable Sounds", "DLS");
                    case 0x0b:
                        return i18nc("File Reference", "File Ref.");
                    case 0x7b:
                        return i18n("End of File");
                    case 0x7c:
                        return i18n("Wait");
                    case 0x7d:
                        return i18n("Cancel");
                    case 0x7e:
                        return i18n("NAK");
                    case 0x7f:
                        return i18n("ACK");
                    default:
                        break;
                }
            } else
            if (msgId == 0x7f) { // Universal Real Time
                switch (subId1) {
                    case 0x01:
                        return i18n("MTC");
                    case 0x02:
                        return i18n("Show Control");
                    case 0x03:
                        return i18n("Notation");
                    case 0x04:
                        return i18n("Device Control");
                    case 0x05:
                        return i18n("MTC Cueing");
                    case 0x06:
                        return i18n("MMC Command");
                    case 0x07:
                        return i18n("MMC Response");
                    case 0x08:
                        return i18n("Tuning");
                    case 0x09:
                        return i18nc("General MIDI 2 Controller Destination", "GM2 Destination");
                    case 0x0a:
                        return i18nc("Key-based Instrument Control", "Instrument");
                    case 0x0b:
                        return i18nc("Scalable Polyphony MIDI MIP Message", "Polyphony");
                    case 0x0c:
                        return i18nc("Mobile Phone Control Message", "Mobile Phone");
                    default:
                        break;
                }
            }
        }
        return QString::number(sev->getLength());
    }
    return QString();
}

QString
SequenceModel::sysex_mtc(const int id) const
{
    switch (id) {
        case 0x00:
            return i18nc("MTC special setup","Special");
        case 0x01:
            return i18n("Punch In Points");
        case 0x02:
            return i18n("Punch Out Points");
        case 0x03:
            return i18n("Delete Punch In Points");
        case 0x04:
            return i18n("Delete Punch Out Points");
        case 0x05:
            return i18n("Event Start Point");
        case 0x06:
            return i18n("Event Stop Point");
        case 0x07:
            return i18n("Event Start Point With Info");
        case 0x08:
            return i18n("Event Stop Point With Info");
        case 0x09:
            return i18n("Delete Event Start Point");
        case 0x0a:
            return i18n("Delete Event Stop Point");
        case 0x0b:
            return i18n("Cue Points");
        case 0x0c:
            return i18n("Cue Points With Info");
        case 0x0d:
            return i18n("Delete Cue Point");
        case 0x0e:
            return i18n("Event Name");
        default:
            break;
    }
    return QString();
}

QString
SequenceModel::sysex_mmc(const int id) const
{
    switch (id) {
    case 0x01:
        return i18n("Stop");
    case 0x02:
        return i18n("Play");
    case 0x03:
        return i18n("Deferred play");
    case 0x04:
        return i18n("Fast forward");
    case 0x05:
        return i18n("Rewind");
    case 0x06:
        return i18n("Punch in");
    case 0x07:
        return i18n("Punch out");
    case 0x08:
        return i18n("Pause recording");
    case 0x09:
        return i18n("Pause");
    case 0x0a:
        return i18n("Eject");
    case 0x0b:
        return i18n("Chase");
    case 0x0c:
        return i18n("Error reset");
    case 0x0d:
        return i18n("Reset");
    case 0x40:
        return i18n("Write");
    case 0x41:
        return i18n("Masked Write");
    case 0x42:
        return i18n("Read");
    case 0x43:
        return i18n("Update");
    case 0x44:
        return i18n("Locate");
    case 0x45:
        return i18n("Variable play");
    case 0x46:
        return i18n("Search");
    case 0x47:
        return i18n("Shuttle");
    case 0x48:
        return i18n("Step");
    default:
        break;
    }
    return QString();
}

QString
SequenceModel::sysex_data2(const SequencerEvent *ev) const
{
    const SysExEvent *sev = static_cast<const SysExEvent*>(ev);
    if (sev != NULL) {
        if (m_translateSysex) {
            unsigned char *ptr = (unsigned char *) sev->getData();
            if (sev->getLength() < 6) return QString();
            if (ptr[0] != 0xf0) return QString();
            int msgId = ptr[1];
            int subId1 = ptr[3];
            int subId2 = ptr[4];
            if (msgId == 0x7e) { // Universal Non Real Time
                switch (subId1) {
                    case 0x01:
                        return i18n("Header");
                    case 0x02:
                        return i18n("Data Packet");
                    case 0x03:
                        return i18n("Request");
                    case 0x04:
                        return sysex_mtc(subId2);
                        break;
                    case 0x05:
                        switch (subId2) {
                            case 0x01:
                                return i18n("Loop Points Send");
                            case 0x02:
                                return i18n("Loop Points Request");
                            case 0x03:
                                return i18n("Sample Name Send");
                            case 0x04:
                                return i18n("Sample Name Request");
                            case 0x05:
                                return i18n("Ext.Dump Header");
                            case 0x06:
                                return i18n("Ext.Loop Points Send");
                            case 0x07:
                                return i18n("Ext.Loop Points Request");
                            default:
                                break;
                        }
                        break;
                    case 0x06:
                        switch (subId2) {
                            case 0x01:
                                return i18n("Identity Request");
                            case 0x02:
                                return i18n("Identity Reply");
                            default:
                                break;
                        }
                        break;
                    case 0x07:
                        switch (subId2) {
                            case 0x01:
                                return i18n("Header");
                            case 0x02:
                                return i18n("Data Packet");
                            case 0x03:
                                return i18n("Request");
                            default:
                                break;
                        }
                        break;
                    case 0x08:
                        switch (subId2) {
                            case 0x00:
                                return i18n("Dump Request");
                            case 0x01:
                                return i18n("Bulk Dump");
                            case 0x02:
                                return i18n("Note Change");
                            case 0x03:
                                return i18n("Tuning Dump Request");
                            case 0x04:
                                return i18n("Key-based Tuning Dump");
                            case 0x05:
                                return i18n("Scale/Octave Dump 1b");
                            case 0x06:
                                return i18n("Scale/Octave Dump 2b");
                            case 0x07:
                                return i18n("Single Note Change");
                            case 0x08:
                                return i18n("Scale/Octave Tuning 1b");
                            case 0x09:
                                return i18n("Scale/Octave Tuning 2b");
                            default:
                                break;
                        }
                        break;
                    case 0x09:
                        switch (subId2) {
                            case 0x01:
                                return i18n("GM On");
                            case 0x02:
                                return i18n("GM Off");
                            case 0x03:
                                return i18n("GM2 On");
                            default:
                                break;
                        }
                        break;
                    case 0x0a:
                        switch (subId2) {
                            case 0x01:
                                return i18n("DLS On");
                            case 0x02:
                                return i18n("DLS Off");
                            case 0x03:
                                return i18n("DLS Voice Alloc. Off");
                            case 0x04:
                                return i18n("DLS Voice Alloc. On");
                            default:
                                break;
                        }
                        break;
                    case 0x0b:
                        switch (subId2) {
                            case 0x01:
                                return i18n("Open");
                            case 0x02:
                                return i18n("Select Contents");
                            case 0x03:
                                return i18n("Open and Select");
                            case 0x04:
                                return i18n("Close");
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
                return QString::number(subId2,16);
            } else
            if (msgId == 0x7f) { // Universal Real Time
                switch (subId1) {
                    case 0x01:
                        switch (subId2) {
                            case 0x01:
                                return i18n("Full Frame");
                            case 0x02:
                                return i18n("User Bits");
                            default:
                                break;
                        }
                        break;
                    case 0x02:
                        switch (subId2) {
                            case 0x00:
                                return i18n("MSC Extension");
                            default:
                                return i18n("MSC Cmd.%1", subId2);
                        }
                        break;
                    case 0x03:
                        switch (subId2) {
                            case 0x01:
                                return i18n("Bar Marker");
                            case 0x02:
                            case 0x42:
                                return i18n("Time Signature");
                            default:
                                break;
                        }
                        break;
                    case 0x04:
                        switch (subId2) {
                            case 0x01:
                                return i18nc("sound volume","Volume");
                            case 0x02:
                                return i18nc("sound balance","Balance");
                            case 0x03:
                                return i18n("Fine Tuning");
                            case 0x04:
                                return i18n("Coarse Tuning");
                            case 0x05:
                                return i18n("Global Parameter");
                            default:
                                break;
                        }
                        break;
                    case 0x05:
                        return sysex_mtc(subId2);
                    case 0x06:
                        return sysex_mmc(subId2);
                    case 0x07:
                        return i18n("Response %1", subId2);
                    case 0x08:
                        switch (subId2) {
                            case 0x02:
                                return i18n("Single Note");
                            case 0x07:
                                return i18n("Single Note with Bank");
                            case 0x08:
                                return i18n("Scale/Octave 1b");
                            case 0x09:
                                return i18n("Scale/Octave 2b");
                            default:
                                break;
                        }
                        break;
                    case 0x09:
                        switch (subId2) {
                            case 0x01:
                                return i18n("Channel aftertouch");
                            case 0x02:
                                return i18n("Polyphonic aftertouch");
                            case 0x03:
                                return i18n("Controller");
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
                return QString::number(subId2,16);
            }
        }
    }
    return QString();
}

int
SequenceModel::sysex_data_first(const SequencerEvent *ev) const
{
    const SysExEvent *sev = static_cast<const SysExEvent*>(ev);
    int result = 0;
    if (sev != NULL) {
        unsigned char *ptr = (unsigned char *) sev->getData();
        if (sev->getLength() < 6) return result;
        if (ptr[0] != 0xf0) return result;
        int msgId = ptr[1];
        int subId1 = ptr[3];
        if (msgId == 0x7e) { // Universal Non Real Time
            if (subId1 >= 0x04 && subId1 <= 0x0b)
                result = 5;
            else
                result = 4;
        } else
        if (msgId == 0x7f) // Universal Real Time
            result = 5;
    }
    return result;
}

QString
SequenceModel::sysex_data3(const SequencerEvent *ev) const
{
    const SysExEvent *sev = static_cast<const SysExEvent*>(ev);
    if (sev != NULL) {
        QString text;
        unsigned char *data = (unsigned char *) sev->getData();
        unsigned int i, first = 0, last = sev->getLength();
        if (m_translateSysex) {
            first = sysex_data_first(ev);
            if (first != 0)
                last--;
        }
        for (i = first; i < last; ++i) {
            QString h = QString::number(data[i], 16);
            if (i > first)
                text.append(' ');
            text.append(h.rightJustified(2, QChar('0')));
        }
        return text.trimmed();
    }
    return QString();
}

QString
SequenceModel::event_ticks(const SequencerEvent *ev) const
{
    return QString::number(ev->getTick());
}

QString
SequenceModel::event_time(const SequenceItem& itm) const
{
    return QString::number(itm.getSeconds(), 'f', 4);
}

QString
SequenceModel::client_name(const int client_number) const
{
    if (m_showClientNames) {
        QString name = m_clients[client_number];
        if (!name.isEmpty())
            return name;
    }
    return QString::number(client_number);
}

QString
SequenceModel::event_source(const SequencerEvent *ev) const
{
    return QString("%1:%2").arg(client_name(ev->getSourceClient()))
        .arg(ev->getSourcePort());
}

QString
SequenceModel::event_addr(const SequencerEvent *ev) const
{
    const PortEvent* pe = static_cast<const PortEvent*>(ev);
    if (pe != NULL)
        return QString("%1:%2").arg(client_name(pe->getClient()))
                               .arg(pe->getPort());
    else
        return QString();
}

QString
SequenceModel::event_client(const SequencerEvent *ev) const
{
    const ClientEvent* ce = static_cast<const ClientEvent*>(ev);
    if (ce != NULL)
        return client_name(ce->getClient());
    else
        return QString();
}

QString
SequenceModel::event_sender(const SequencerEvent *ev) const
{
    const SubscriptionEvent* se = static_cast<const SubscriptionEvent*>(ev);
    if (se != NULL)
        return QString("%1:%2").arg(client_name(se->getSenderClient()))
                               .arg(se->getSenderPort());
    else
        return QString();
}

QString
SequenceModel::event_dest(const SequencerEvent *ev) const
{
    const SubscriptionEvent* se = static_cast<const SubscriptionEvent*>(ev);
    if (se != NULL)
        return QString("%1:%2").arg(client_name(se->getDestClient()))
                               .arg(se->getDestPort());
    else
        return QString();
}

QString
SequenceModel::common_param(const SequencerEvent *ev) const
{
    const ValueEvent* ve = static_cast<const ValueEvent*>(ev);
    if (ve != NULL)
        return QString::number(ve->getValue());
    else
        return QString();
}

QString
SequenceModel::event_kind(const SequencerEvent *ev) const
{
    QString res;
    if (ev->getSequencerType() == SND_SEQ_EVENT_SYSEX)
        return sysex_type(ev);
    if (m_filter != NULL)
        res = m_filter->getName(ev->getSequencerType());
    if (res.isEmpty())
        res = i18n("Event type %1", ev->getSequencerType());
    return res;
}

QString
SequenceModel::event_channel(const SequencerEvent *ev) const
{
    if (SequencerEvent::isChannel(ev)) {
        const ChannelEvent* che = static_cast<const ChannelEvent*>(ev);
        if (che != NULL)
            return QString::number(che->getChannel()+1);
    } else
        if (ev->getSequencerType() == SND_SEQ_EVENT_SYSEX)
            return sysex_chan(ev);
    return QString();
}

QString
SequenceModel::note_name(const int note) const
{
    const QString m_names_s[] = {i18n("C"), i18n("C♯"), i18n("D"), i18n("D♯"), i18n("E"),
    		i18n("F"), i18n("F♯"), i18n("G"), i18n("G♯"), i18n("A"), i18n("A♯"), i18n("B")};
    const QString m_names_f[] = {i18n("C"), i18n("D♭"), i18n("D"), i18n("E♭"), i18n("E"),
    		i18n("F"), i18n("G♭"), i18n("G"), i18n("A♭"), i18n("A"), i18n("B♭"), i18n("B")};
    int num = note % 12;
    int oct = (note / 12) - 1;
    QString name = m_useFlats ? m_names_f[num] : m_names_s[num];
    return QString("%1%2:%3").arg(name).arg(oct).arg(note);
}

QString
SequenceModel::note_key(const SequencerEvent* ev) const
{
    const KeyEvent* ke = static_cast<const KeyEvent*>(ev);
    if (ke != NULL)
    	if (m_translateNotes)
            if ((ke->getChannel() == MIDI_GM_DRUM_CHANNEL) && (m_ins2 != NULL)) {
                int b = m_lastBank[ke->getChannel()];
                int p = m_lastPatch[ke->getChannel()];
                const InstrumentData& notes = m_ins2->notes(b, p);
                if (notes.contains(ke->getKey()))
                    return QString("%1:%2").arg(notes[ke->getKey()]).arg(ke->getKey());
                else
                    return note_name(ke->getKey());
            } else
                return note_name(ke->getKey());
    	else
            return QString::number(ke->getKey());
    else
        return QString();
}

QString
SequenceModel::program_number(const SequencerEvent* ev) const
{
    const ProgramChangeEvent* pc = static_cast<const ProgramChangeEvent*>(ev);
    if (pc != NULL) {
        if (m_translateCtrls) {
            if (pc->getChannel() == MIDI_GM_DRUM_CHANNEL && m_ins2 != NULL) {
                const InstrumentData& patch = m_ins2->patch(m_lastBank[pc->getChannel()]);
                if (patch.contains(pc->getValue()))
                    return QString("%1:%2").arg(patch[pc->getValue()]).arg(pc->getValue());
            }
            if (pc->getChannel() != MIDI_GM_DRUM_CHANNEL && m_ins != NULL) {
                const InstrumentData& patch = m_ins->patch(m_lastBank[pc->getChannel()]);
                if (patch.contains(pc->getValue()))
                    return QString("%1:%2").arg(patch[pc->getValue()]).arg(pc->getValue());
            }
        }
        return QString::number(pc->getValue());
    }
    return QString();
}

QString
SequenceModel::note_velocity(const SequencerEvent* ev) const
{
    const KeyEvent* ke = static_cast<const KeyEvent*>(ev);
    if (ke != NULL)
        return QString::number(ke->getVelocity());
    else
        return QString();
}

QString
SequenceModel::note_duration(const SequencerEvent* ev) const
{
    const NoteEvent* ne = static_cast<const NoteEvent*>(ev);
    if (ne != NULL)
        return QString::number(ne->getDuration());
    else
        return QString();
}

QString
SequenceModel::control_param(const SequencerEvent* ev) const
{
    const ControllerEvent* ce = static_cast<const ControllerEvent*>(ev);
    if (ce != NULL) {
        if (m_translateCtrls) {
            Instrument* ins = NULL;
            if (ce->getChannel() == MIDI_GM_DRUM_CHANNEL && m_ins2 != NULL)
                ins = m_ins2;
            if (m_ins != NULL)
                ins = m_ins;
            if (ins != NULL) {
                const InstrumentData& ctls = ins->control();
                if (ctls.contains(ce->getParam()))
                    return QString("%1:%2").arg(ctls[ce->getParam()]).arg(ce->getParam());
            }
        }
        return QString::number(ce->getParam());
    }
    return QString();
}

QString
SequenceModel::control_value(const SequencerEvent* ev) const
{
    const ControllerEvent* ce = static_cast<const ControllerEvent*>(ev);
    if (ce != NULL)
        return QString::number(ce->getValue());
    else
        return QString();
}

QString
SequenceModel::pitchbend_value(const SequencerEvent* ev) const
{
    const PitchBendEvent* pe = static_cast<const PitchBendEvent*>(ev);
    if (pe != NULL)
        return QString::number(pe->getValue());
    else
        return QString();
}

QString
SequenceModel::chanpress_value(const SequencerEvent* ev) const
{
    const ChanPressEvent* cp = static_cast<const ChanPressEvent*>(ev);
    if (cp != NULL)
        return QString::number(cp->getValue());
    else
        return QString();
}

QString
SequenceModel::tempo_bpm(const SequencerEvent *ev) const
{
    const TempoEvent* te = static_cast<const TempoEvent*>(ev);
    if (te != NULL)
        return i18n("%1 bpm", QString::number(6e7 / te->getValue(), 'f', 1));
    else
        return QString();
}

QString
SequenceModel::tempo_npt(const SequencerEvent *ev) const
{
    const TempoEvent* te = static_cast<const TempoEvent*>(ev);
    if (te != NULL)
        return QString::number(te->getValue());
    else
        return QString();
}

QString
SequenceModel::text_type(const SequencerEvent *ev) const
{
    const TextEvent* te = static_cast<const TextEvent*>(ev);
    if (te != NULL) {
        switch ( te->getTextType() ) {
        case 1:
            return i18n("Text:1");
        case 2:
            return i18n("Copyright:2");
        case 3:
            return i18nc("song or track name","Name:3");
        case 4:
            return i18n("Instrument:4");
        case 5:
            return i18n("Lyric:5");
        case 6:
            return i18n("Marker:6");
        case 7:
            return i18n("Cue:7");
        default:
            return QString::number( te->getTextType() );
        }
    } else {
        return QString();
    }
}

QString
SequenceModel::text_data(const SequencerEvent *ev) const
{
    const TextEvent* te = static_cast<const TextEvent*>(ev);
    if (te != NULL)
        return te->getText();
    else
        return QString();
}

QString
SequenceModel::time_sig1(const SequencerEvent *ev) const
{
    return i18n("%1/%2",
            ev->getRaw8(0),
            pow(2, ev->getRaw8(1)) );
}

QString
SequenceModel::time_sig2(const SequencerEvent *ev) const
{
    return i18n("%1 clocks per click, %2 32nd per quarter",
            ev->getRaw8(2),
            ev->getRaw8(3) );
}

QString
SequenceModel::key_sig1(const SequencerEvent *ev) const
{
    signed char s = (signed char) ev->getRaw8(0);
    return i18n("%1%2", abs(s),
            s < 0 ? QChar(0x266D) : QChar(0x266F)); //s < 0 ? "♭" : "♯"
}

QString
SequenceModel::key_sig2(const SequencerEvent *ev) const
{
    static QString tmaj[] = {i18n("C flat"), i18n("G flat"), i18n("D flat"),
            i18n("A flat"), i18n("E flat"), i18n("B flat"),i18n("F"),
            i18n("C"), i18n("G"), i18n("D"), i18n("A"),
            i18n("E"), i18n("B"), i18n("F sharp"),i18n("C sharp")};
    static QString tmin[] = {i18n("A flat"), i18n("E flat"), i18n("B flat"),
            i18n("F"), i18n("C"), i18n("G"), i18n("D"),
            i18n("A"), i18n("E"), i18n("B"), i18n("F sharp"), i18n("C sharp"),
            i18n("G sharp"), i18n("D sharp"), i18n("A sharp")};
    signed char s = (signed char) ev->getRaw8(0);
    QString tone, mode;
    if (abs(s) < 8) {
        tone = ( ev->getRaw8(1) == 0 ? tmaj[s + 7] : tmin[s + 7] );
        mode = ( ev->getRaw8(1) == 0 ? i18nc("major mode scale","major") :
                                       i18nc("minor mode scale","minor") );
    }
    return tone + ' ' + mode;
}

QString
SequenceModel::smpte(const SequencerEvent *ev) const
{
    return i18n( "%1:%2:%3:%4:%5",
                 ev->getRaw8(0),
                 ev->getRaw8(1),
                 ev->getRaw8(2),
                 ev->getRaw8(3),
                 ev->getRaw8(4));
}

QString
SequenceModel::var_event(const SequencerEvent *ev) const
{
    const VariableEvent* ve = static_cast<const VariableEvent*>(ev);
    if (ve != NULL) {
        unsigned int i = 0;
        const unsigned char *data = reinterpret_cast<const unsigned char *>(ve->getData());
        QString text;
        if (ve->getSequencerType() == SND_SEQ_EVENT_USR_VAR2)
            i = 1;
        for ( ; i < ve->getLength(); ++i ) {
            QString h = QString::number(data[i], 16);
            text.append(' ');
            text.append(h.rightJustified(2, QChar('0')));
        }
        return text.trimmed();
    }
    return QString();
}

QString
SequenceModel::meta_misc(const SequencerEvent *ev) const
{
    const VariableEvent* ve = static_cast<const VariableEvent*>(ev);
    if (ve != NULL) {
        int type = ve->getData()[0];
        return QString::number(type);
    }
    return QString();
}

QString
SequenceModel::event_data1(const SequencerEvent *ev) const
{
    switch (ev->getSequencerType()) {
    /* MIDI Channel events */
    case SND_SEQ_EVENT_NOTE:
    case SND_SEQ_EVENT_NOTEON:
    case SND_SEQ_EVENT_NOTEOFF:
    case SND_SEQ_EVENT_KEYPRESS:
        return note_key(ev);

    case SND_SEQ_EVENT_PGMCHANGE:
        return program_number(ev);

    case SND_SEQ_EVENT_PITCHBEND:
        return pitchbend_value(ev);

    case SND_SEQ_EVENT_CHANPRESS:
        return chanpress_value(ev);

    case SND_SEQ_EVENT_CONTROLLER:
    case SND_SEQ_EVENT_CONTROL14:
    case SND_SEQ_EVENT_NONREGPARAM:
    case SND_SEQ_EVENT_REGPARAM:
        return control_param(ev);

        /* MIDI Common events */
    case SND_SEQ_EVENT_SYSEX:
        return sysex_data1(ev);

    case SND_SEQ_EVENT_SONGPOS:
    case SND_SEQ_EVENT_SONGSEL:
    case SND_SEQ_EVENT_QFRAME:
        return common_param(ev);

        /* ALSA Client/Port events */
    case SND_SEQ_EVENT_PORT_START:
    case SND_SEQ_EVENT_PORT_EXIT:
    case SND_SEQ_EVENT_PORT_CHANGE:
        return event_addr(ev);

    case SND_SEQ_EVENT_CLIENT_START:
    case SND_SEQ_EVENT_CLIENT_EXIT:
    case SND_SEQ_EVENT_CLIENT_CHANGE:
        return event_client(ev);

    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
       return event_sender(ev);

    case SND_SEQ_EVENT_TEMPO:
        return tempo_npt(ev);

    case SND_SEQ_EVENT_USR_VAR0:
        return text_type(ev);

    case SND_SEQ_EVENT_USR1:
    case SND_SEQ_EVENT_USR2:
    case SND_SEQ_EVENT_USR3:
        return QString::number(ev->getRaw8(0));

    case SND_SEQ_EVENT_USR_VAR2:
        return meta_misc(ev);

       /* Other events */
    default:
        return QString();
    }
    return QString();
}

QString
SequenceModel::event_data2(const SequencerEvent *ev) const
{
    switch (ev->getSequencerType()) {
    /* MIDI Channel events */

    case SND_SEQ_EVENT_NOTE:
    case SND_SEQ_EVENT_NOTEON:
    case SND_SEQ_EVENT_NOTEOFF:
    case SND_SEQ_EVENT_KEYPRESS:
        return note_velocity(ev);

    case SND_SEQ_EVENT_CONTROLLER:
    case SND_SEQ_EVENT_CONTROL14:
    case SND_SEQ_EVENT_NONREGPARAM:
    case SND_SEQ_EVENT_REGPARAM:
        return control_value(ev);

    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
       return event_dest(ev);

    case SND_SEQ_EVENT_SYSEX:
        return sysex_data2(ev);

    case SND_SEQ_EVENT_TEMPO:
        return tempo_bpm(ev);

    case SND_SEQ_EVENT_TIMESIGN:
        return time_sig1(ev);

    case SND_SEQ_EVENT_KEYSIGN:
        return key_sig1(ev);

        /* Other events */
    default:
        return QString();
    }
    return QString();
}

QString
SequenceModel::event_data3(const SequencerEvent *ev) const
{
    switch (ev->getSequencerType()) {

    case SND_SEQ_EVENT_NOTE:
        return note_duration(ev);

    case SND_SEQ_EVENT_SYSEX:
        return sysex_data3(ev);

    case SND_SEQ_EVENT_TIMESIGN:
        return time_sig2(ev);

    case SND_SEQ_EVENT_KEYSIGN:
        return key_sig2(ev);

    case SND_SEQ_EVENT_USR_VAR0:
        return text_data(ev);

    case SND_SEQ_EVENT_USR4:
        return smpte(ev);

    case SND_SEQ_EVENT_USR_VAR1:
    case SND_SEQ_EVENT_USR_VAR2:
        return var_event(ev);

    }
    return QString();
}

const SequenceItem*
SequenceModel::getItem(const int row) const
{
    if (!m_items.isEmpty() && (row >= 0) && (row < m_items.size()))
        return &m_items[row];
    return NULL;
}

const SequencerEvent*
SequenceModel::getEvent(const int row) const
{
    if (!m_items.isEmpty() && (row >= 0) && (row < m_items.size()))
        return m_items[row].getEvent();
    return NULL;
}

void
SequenceModel::loadFromFile(const QString& path)
{
    clear();
    m_tempSong.clear();
    m_currentTrack = -1;
    m_initialTempo = -1;
    KMimeType::Ptr type = KMimeType::findByPath(path);
    if (type->name() == "audio/midi") {
        m_appendFunc = &SequenceModel::appendSMFEvent;
        m_reportsFilePos = true;
        m_smf->readFromFile(path);
    } else if (type->name() == "audio/cakewalk") {
        m_appendFunc = &SequenceModel::appendWRKEvent;
        m_reportsFilePos = true;
        m_wrk->readFromFile(path);
    } else if (type->name() == "audio/overture") {
        m_appendFunc = &SequenceModel::appendOVEEvent;
        m_reportsFilePos = false;
        m_ove->readFromFile(path);
    } else {
        m_appendFunc = 0;
        m_reportsFilePos = false;
        kDebug() << "unrecognized format:" << type->name();
        return;
    }
    m_tempSong.sort();
    beginInsertRows(QModelIndex(), 0, m_tempSong.count() - 1);
    m_items += m_tempSong;
    endInsertRows();
    m_items.setLast(m_tempSong.getLast());
    m_tempSong.clear();
}

void
SequenceModel::processItems()
{
    m_tempSong.clear();
    foreach ( const SequenceItem& itm, m_items ) {
        SequencerEvent* ev = itm.getEvent();
        double seconds = 0;
        long ticks = itm.getTicks();
        int track = itm.getTrack();
        if ( ev != NULL && ev->getSequencerType() == SND_SEQ_EVENT_NOTE ) {
            NoteEvent* note = static_cast<NoteEvent*>(ev);
            NoteOnEvent* noteon = new NoteOnEvent( note->getChannel(),
                                                   note->getKey(),
                                                   note->getVelocity() );
            noteon->scheduleTick(m_queueId, ticks, false);
            noteon->setTag(track);
            SequenceItem itm1(seconds, ticks, track, noteon);
            m_tempSong.append(itm1);
            NoteOffEvent* noteoff = new NoteOffEvent( note->getChannel(),
                                                      note->getKey(),
                                                      note->getVelocity() );
            ticks += note->getDuration();
            noteoff->scheduleTick(m_queueId, ticks, false);
            noteoff->setTag(track);
            SequenceItem itm2(seconds, ticks, track, noteoff);
            m_tempSong.append(itm2);
        } else {
            SequenceItem itm(seconds, ticks, track, ev->clone());
            m_tempSong.append(itm);
        }
    }
    m_tempSong.sort();
}

void
SequenceModel::saveToFile(const QString& path)
{
    QFileInfo info(path);
    if (info.suffix().toLower() == "txt") {
        QFile file(path);
        file.open(QIODevice::WriteOnly);
        QTextStream stream(&file);
        saveToTextStream(stream);
        file.close();
    } else { // MIDI
        processItems();
        m_smf->setDivision(m_division);
        m_smf->setFileFormat(1);
        m_smf->setTracks(m_ntrks);
        m_smf->writeToFile(path);
        QList<SequenceItem>::Iterator it;
        for ( it = m_tempSong.begin(); it != m_tempSong.end(); ++it )
            (*it).deleteEvent();
        m_tempSong.clear();
    }
}

void
SequenceModel::appendEvent(long ticks, double seconds, int track, SequencerEvent* ev)
{
    if (seconds > m_duration)
        m_duration = seconds;
    ev->setSource(m_portId);
    ev->scheduleTick(m_queueId, ticks, false);
    ev->setTag(track);
    if (ev->getSequencerType() != SND_SEQ_EVENT_TEMPO) {
        ev->setSubscribers();
    }
    SequenceItem itm(seconds, ticks, track, ev);
    m_tempSong.append(itm);
    m_tempSong.setLast(ticks);
}

void
SequenceModel::appendSMFEvent(long, int, SequencerEvent* ev)
{
    long ticks = m_smf->getCurrentTime();
    double seconds = m_smf->getRealTime() / 1600.0;
    appendEvent(ticks, seconds, m_currentTrack, ev);
    if (m_reportsFilePos)
        emit loadProgress(m_smf->getFilePos());
    KApplication::processEvents();
}

void
SequenceModel::headerEvent(int format, int ntrks, int division)
{
    m_format = format;
    m_ntrks = ntrks;
    m_division = division;
    m_fileFormat = i18n("SMF type %1", format);
}

void
SequenceModel::trackStartEvent()
{
    m_currentTrack++;
    TrackMapRec rec;
    rec.channel = -1;
    rec.pitch = 0;
    rec.velocity = 0;
    m_trackMap[m_currentTrack] = rec;
    m_tempSong.setMutedState(m_currentTrack, false);
    if (m_reportsFilePos)
        emit loadProgress(m_smf->getFilePos());
}

void
SequenceModel::trackEndEvent()
{
    long ticks = m_smf->getCurrentTime();
    m_tempSong.setLast(ticks);
    if (m_reportsFilePos)
        emit loadProgress(m_smf->getFilePos());
}

void
SequenceModel::endOfTrackEvent()
{
    double seconds = m_smf->getRealTime() / 1600.0;
    if (seconds > m_duration)
        m_duration = seconds;
    if (m_reportsFilePos)
        emit loadProgress(m_smf->getFilePos());
}

void
SequenceModel::noteOnEvent(int chan, int pitch, int vol)
{
    SequencerEvent* ev = new NoteOnEvent(chan, pitch, vol);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::noteOffEvent(int chan, int pitch, int vol)
{
    SequencerEvent* ev = new NoteOffEvent(chan, pitch, vol);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::keyPressEvent(int chan, int pitch, int press)
{
    SequencerEvent* ev = new KeyPressEvent(chan, pitch, press);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::ctlChangeEvent(int chan, int ctl, int value)
{
    if (ctl == MIDI_CTL_MSB_BANK ||
        ctl == MIDI_CTL_LSB_BANK ) {

        int bsm = 0;
        if (chan == 9 && m_ins2 != NULL)
            bsm = m_ins2->bankSelMethod();
        else if (m_ins != NULL)
            bsm = m_ins->bankSelMethod();

        if (ctl == MIDI_CTL_MSB_BANK) {
            m_lastCtlMSB = value;
            m_lastCtlLSB = 0;
        }
        if (ctl == MIDI_CTL_LSB_BANK)
            m_lastCtlLSB = value;

        switch(bsm) {
            case 0:
                m_lastBank[chan] = m_lastCtlMSB << 7 | m_lastCtlLSB;
                break;
            case 1:
                m_lastBank[chan] = value;
                break;
            case 2:
                m_lastBank[chan] = value;
                break;
        }
    }
}

void
SequenceModel::smfCtlChangeEvent(int chan, int ctl, int value)
{
    ctlChangeEvent(chan, ctl, value);
    SequencerEvent* ev = new ControllerEvent(chan, ctl, value);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::pitchBendEvent(int chan, int value)
{
    SequencerEvent* ev = new PitchBendEvent(chan, value);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::programEvent(int chan, int patch)
{
    m_lastPatch[chan] = patch;
    SequencerEvent* ev = new ProgramChangeEvent(chan, patch);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::chanPressEvent(int chan, int press)
{
    SequencerEvent* ev = new ChanPressEvent(chan, press);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::sysexEvent(const QByteArray& data)
{
    SequencerEvent* ev = new SysExEvent(data);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::seqSpecificEvent(const QByteArray& data)
{
    SequencerEvent* ev = new VariableEvent(data);
    ev->setSequencerType(SND_SEQ_EVENT_USR_VAR1);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::metaMiscEvent(int typ, const QByteArray& data)
{
    QByteArray dataCopy;
    dataCopy.append(typ);
    dataCopy.append(data);
    SequencerEvent* ev = new VariableEvent(dataCopy);
    ev->setSequencerType(SND_SEQ_EVENT_USR_VAR2);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::seqNum(int seq)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_USR1);
    ev->setRaw8(0, seq);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::forcedChannel(int channel)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_USR2);
    ev->setRaw8(0, channel);
    (this->*m_appendFunc)(0, 0, ev);
    TrackMapRec rec = m_trackMap[m_currentTrack];
    rec.channel = channel;
    m_trackMap[m_currentTrack] = rec;
}

void
SequenceModel::forcedPort(int port)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_USR3);
    ev->setRaw8(0, port);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::smpteEvent(int b0, int b1, int b2, int b3, int b4)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_USR4);
    ev->setRaw8(0, b0);
    ev->setRaw8(1, b1);
    ev->setRaw8(2, b2);
    ev->setRaw8(3, b3);
    ev->setRaw8(4, b4);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::timeSigEvent(int b0, int b1, int b2, int b3)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_TIMESIGN);
    ev->setRaw8(0, b0);
    ev->setRaw8(1, b1);
    ev->setRaw8(2, b2);
    ev->setRaw8(3, b3);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::keySigEventSMF(int b0, int b1)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_KEYSIGN);
    ev->setRaw8(0, b0);
    ev->setRaw8(1, b1);
    (this->*m_appendFunc)(0, 0, ev);
    m_useFlats = (b0 < 0);
}

void
SequenceModel::textEvent(int type, const QString& data)
{
    SequencerEvent* ev = new TextEvent(data, type);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::tempoEvent(int tempo)
{
    if ( m_initialTempo < 0 )
    {
        m_initialTempo = round( 6e7 / tempo );
    }
    SequencerEvent* ev = new TempoEvent(m_queueId, tempo);
    (this->*m_appendFunc)(0, 0, ev);
}

void
SequenceModel::errorHandlerSMF(const QString& errorStr)
{
    m_loadingMessages.append(errorStr);
    if (m_reportsFilePos)
        m_loadingMessages.append(
            i18n(" at offset %1",m_smf->getFilePos()));
}

void
SequenceModel::trackHandler(int track)
{
    unsigned int delta, last_tick = 0;
    foreach ( const SequenceItem& itm, m_tempSong ) {
        if (itm.getTrack() == track) {
            const SequencerEvent* ev = itm.getEvent();
            if (ev != NULL) {
                delta = ev->getTick() - last_tick;
                last_tick = ev->getTick();
                switch(ev->getSequencerType()) {
                    case SND_SEQ_EVENT_TEMPO: {
                        const TempoEvent* e = static_cast<const TempoEvent*>(ev);
                        if (e != NULL)
                            m_smf->writeTempo(delta, e->getValue());
                    }
                    break;
                    case SND_SEQ_EVENT_USR_VAR0: {
                        const TextEvent* e = static_cast<const TextEvent*>(ev);
                        if (e != NULL)
                            m_smf->writeMetaEvent(delta, e->getTextType(),
                                                  e->getText());
                    }
                    break;
                    case SND_SEQ_EVENT_USR_VAR1: {
                        const VariableEvent* e = static_cast<const VariableEvent*>(ev);
                        if (e != NULL) {
                            QByteArray b(e->getData(), e->getLength());
                            m_smf->writeMetaEvent(delta, sequencer_specific, b);
                        }
                    }
                    break;
                    case SND_SEQ_EVENT_USR_VAR2: {
                        const VariableEvent* e = static_cast<const VariableEvent*>(ev);
                        if (e != NULL) {
                            QByteArray b(e->getData(), e->getLength());
                            int type = b.at(0);
                            b.remove(0, 1);
                            m_smf->writeMetaEvent(delta, type, b);
                        }
                    }
                    break;
                    case SND_SEQ_EVENT_SYSEX: {
                        const SysExEvent* e = static_cast<const SysExEvent*>(ev);
                        if (e != NULL)
                            m_smf->writeMidiEvent(delta, system_exclusive,
                                    (long) e->getLength(),
                                    (char *) e->getData());
                    }
                    break;
                    case SND_SEQ_EVENT_NOTEON: {
                        const KeyEvent* e = static_cast<const KeyEvent*>(ev);
                        if (e != NULL)
                            m_smf->writeMidiEvent(delta, note_on,
                                    e->getChannel(),
                                    e->getKey(),
                                    e->getVelocity());
                    }
                    break;
                    case SND_SEQ_EVENT_NOTEOFF: {
                        const KeyEvent* e = static_cast<const KeyEvent*>(ev);
                        if (e != NULL)
                            m_smf->writeMidiEvent(delta, note_off,
                                    e->getChannel(),
                                    e->getKey(),
                                    e->getVelocity());
                    }
                    break;
                    case SND_SEQ_EVENT_KEYPRESS: {
                        const KeyEvent* e = static_cast<const KeyEvent*>(ev);
                        if (e != NULL)
                            m_smf->writeMidiEvent(delta, poly_aftertouch,
                                    e->getChannel(),
                                    e->getKey(),
                                    e->getVelocity());
                    }
                    break;
                    case SND_SEQ_EVENT_CONTROLLER:
                    case SND_SEQ_EVENT_CONTROL14:
                    case SND_SEQ_EVENT_NONREGPARAM:
                    case SND_SEQ_EVENT_REGPARAM: {
                        const ControllerEvent* e = static_cast<const ControllerEvent*>(ev);
                        if (e != NULL)
                            m_smf->writeMidiEvent(delta, control_change,
                                    e->getChannel(),
                                    e->getParam(),
                                    e->getValue());
                    }
                    break;
                    case SND_SEQ_EVENT_PGMCHANGE: {
                        const ProgramChangeEvent* e = static_cast<const ProgramChangeEvent*>(ev);
                        if (e != NULL)
                            m_smf->writeMidiEvent(delta, program_chng,
                                    e->getChannel(), e->getValue());
                    }
                    break;
                    case SND_SEQ_EVENT_CHANPRESS: {
                        const ChanPressEvent* e = static_cast<const ChanPressEvent*>(ev);
                        if (e != NULL)
                            m_smf->writeMidiEvent(delta, channel_aftertouch,
                                    e->getChannel(), e->getValue());
                    }
                    break;
                    case SND_SEQ_EVENT_PITCHBEND: {
                        const PitchBendEvent* e = static_cast<const PitchBendEvent*>(ev);
                        if (e != NULL)
                            m_smf->writeMidiEvent(delta, pitch_wheel,
                                    e->getChannel(), e->getValue());
                    }
                    break;
                    case SND_SEQ_EVENT_TIMESIGN: {
                        m_smf->writeTimeSignature(delta, ev->getRaw8(0),
                                ev->getRaw8(1),
                                ev->getRaw8(2),
                                ev->getRaw8(3));
                        // writeTimeSignature(0, 3, 2, 36, 8) = 3/4
                    }
                    break;
                    case SND_SEQ_EVENT_KEYSIGN: {
                        m_smf->writeKeySignature(delta, ev->getRaw8(0),
                                ev->getRaw8(1));
                        // writeKeySignature(0, 2, major_mode) = D major (2#)
                    }
                    break;
                    case SND_SEQ_EVENT_USR1: {
                        m_smf->writeSequenceNumber(delta, ev->getRaw8(0));
                    }
                    break;
                    case SND_SEQ_EVENT_USR2: {
                        int data = ev->getRaw8(0);
                        m_smf->writeMetaEvent(delta, forced_channel, data);
                    }
                    case SND_SEQ_EVENT_USR3: {
                        int data = ev->getRaw8(0);
                        m_smf->writeMetaEvent(delta, forced_port, data);
                    }
                    case SND_SEQ_EVENT_USR4: {
                        QByteArray data;
                        data.append(ev->getRaw8(0));
                        data.append(ev->getRaw8(1));
                        data.append(ev->getRaw8(2));
                        data.append(ev->getRaw8(3));
                        data.append(ev->getRaw8(4));
                        m_smf->writeMetaEvent(delta, smpte_offset, data);
                    }
                    break;
                }
            }
        }
    }
    // final event
    delta = m_tempSong.getLast() - last_tick;
    m_smf->writeMetaEvent(delta, end_of_track);
}

QStringList
SequenceModel::getInstruments() const
{
    QStringList lst;
    InstrumentList::ConstIterator it;
    for(it = m_insList.begin(); it != m_insList.end(); ++it) {
        if(!it.key().endsWith(QLatin1String("Drums"), Qt::CaseInsensitive))
            lst += it.key();
    }
    return lst;
}

void
SequenceModel::setInstrumentName(const QString name)
{
    m_instrumentName = name;
    QString drmName = name;
    drmName.append(" Drums");

    if (m_insList.contains(name))
        m_ins = &m_insList[name];
    else
        m_ins = NULL;

    if (m_insList.contains(drmName))
        m_ins2 = &m_insList[drmName];
    else
        m_ins2 = NULL;
}

QString
SequenceModel::getDuration() const
{
    double fractpart, intpart;
    fractpart = modf ( m_duration , &intpart );
    QTime t = QTime(0, 0).addSecs(intpart).addMSecs(ceil(fractpart*1000));
    return t.toString("hh:mm:ss.zzz");
}

void
SequenceModel::setEncoding(const QString& encoding)
{
    if (m_encoding != encoding) {
        QString name = KGlobal::charsets()->encodingForName(encoding);
        QTextCodec* codec = QTextCodec::codecForName(name.toLatin1());
        m_smf->setTextCodec(codec);
        m_wrk->setTextCodec(codec);
        m_encoding = encoding;
    }
}

/* ********************************* *
 * Cakewalk WRK file format handling
 * ********************************* */

void
SequenceModel::appendWRKEvent(long ticks, int track, SequencerEvent* ev)
{
    double seconds = m_wrk->getRealTime(ticks);
    appendEvent(ticks, seconds, track, ev);
    if (m_reportsFilePos)
        emit loadProgress(m_wrk->getFilePos());
    KApplication::processEvents();
}

void SequenceModel::errorHandlerWRK(const QString& errorStr)
{
    m_loadingMessages.append(errorStr);
    if (m_reportsFilePos)
        m_loadingMessages.append(
            i18n(" at offset %1",m_wrk->getFilePos()));
}

void SequenceModel::fileHeader(int verh, int verl)
{
    m_fileFormat = i18n("WRK file version %1.%2", verh, verl);
    m_format = 1;
    m_ntrks = 0;
    m_division = 120;
}

void SequenceModel::timeBase(int timebase)
{
    m_division = timebase;
}

void SequenceModel::globalVars()
{
    emit keySigEventWRK(0, m_wrk->getKeySig());
    m_tempSong.setLast( m_wrk->getEndAllTime() );
    if (m_reportsFilePos)
        emit loadProgress(m_wrk->getFilePos());
}

void SequenceModel::streamEndEvent(long time)
{
    double seconds = m_wrk->getRealTime(time);
    if (seconds > m_duration)
        m_duration = seconds;
}

void SequenceModel::trackHeader( const QString& name1, const QString& name2,
                           int trackno, int channel,
                           int pitch, int velocity, int /*port*/,
                           bool /*selected*/, bool muted, bool /*loop*/ )
{
    m_currentTrack = trackno;
    TrackMapRec rec;
    rec.channel = channel;
    rec.pitch = pitch;
    rec.velocity = velocity;
    m_trackMap[trackno] = rec;
    m_ntrks++;
    m_tempSong.setMutedState(m_currentTrack, muted);
    QString trkName = name1 + ' ' + name2;
    trkName = trkName.trimmed();
    if (!trkName.isEmpty()) {
        SequencerEvent* ev = new TextEvent(trkName, 3);
        (this->*m_appendFunc)(0, trackno, ev);
    }
    if (m_reportsFilePos)
        emit loadProgress(m_wrk->getFilePos());
}
void SequenceModel::noteEvent(int track, long time, int chan, int pitch, int vol, int dur)
{
    int channel = chan;
    TrackMapRec rec = m_trackMap[track];
    int key = pitch + rec.pitch;
    int velocity = vol + rec.velocity;
    if (rec.channel > -1)
        channel = rec.channel;
    SequencerEvent* ev = new NoteEvent(channel, key, velocity, dur);
    (this->*m_appendFunc)(time, track, ev);
}

void SequenceModel::keyPressEvent(int track, long time, int chan, int pitch, int press)
{
    int channel = chan;
    TrackMapRec rec = m_trackMap[track];
    int key = pitch + rec.pitch;
    if (rec.channel > -1)
        channel = rec.channel;
    SequencerEvent* ev = new KeyPressEvent(channel, key, press);
    (this->*m_appendFunc)(time, track, ev);
}

void SequenceModel::wrkCtlChangeEvent(int track, long time, int chan, int ctl, int value)
{
    int channel = chan;
    TrackMapRec rec = m_trackMap[track];
    if (rec.channel > -1)
        channel = rec.channel;
    ctlChangeEvent(channel, ctl, value);
    SequencerEvent* ev = new ControllerEvent(channel, ctl, value);
    (this->*m_appendFunc)(time, track, ev);
}

void SequenceModel::pitchBendEvent(int track, long time, int chan, int value)
{
    int channel = chan;
    TrackMapRec rec = m_trackMap[track];
    if (rec.channel > -1)
        channel = rec.channel;
    SequencerEvent* ev = new PitchBendEvent(channel, value);
    (this->*m_appendFunc)(time, track, ev);
}

void SequenceModel::programEvent(int track, long time, int chan, int patch)
{
    int channel = chan;
    TrackMapRec rec = m_trackMap[track];
    if (rec.channel > -1)
        channel = rec.channel;
    SequencerEvent* ev = new ProgramChangeEvent(channel, patch);
    (this->*m_appendFunc)(time, track, ev);
}

void SequenceModel::chanPressEvent(int track, long time, int chan, int press)
{
    int channel = chan;
    TrackMapRec rec = m_trackMap[track];
    if (rec.channel > -1)
        channel = rec.channel;
    SequencerEvent* ev = new ChanPressEvent(channel, press);
    (this->*m_appendFunc)(time, track, ev);
}

void SequenceModel::sysexEvent(int track, long time, int bank)
{
    SysexEventRec rec;
    rec.track = track;
    rec.time = time;
    rec.bank = bank;
    m_savedSysexEvents.append(rec);
}

void SequenceModel::sysexEventBank(int bank, const QString& /*name*/, bool autosend, int /*port*/, const QByteArray& data)
{
    SysExEvent* ev = new SysExEvent(data);
    if (autosend)
       (this->*m_appendFunc)(0, 0, ev->clone());
    foreach(const SysexEventRec& rec, m_savedSysexEvents) {
        if (rec.bank == bank)
           (this->*m_appendFunc)(rec.time, rec.track, ev->clone());
    }
    delete ev;
}

void SequenceModel::textEvent(int track, long time, int /*type*/, const QString& data)
{
    SequencerEvent* ev = new TextEvent(data, 1);
    (this->*m_appendFunc)(time, track, ev);
}

void SequenceModel::timeSigEvent(int bar, int num, int den)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_TIMESIGN);
    int div, d = den;
    for ( div = 0; d > 1; d /= 2 )
        ++div;
    ev->setRaw8(0, num);
    ev->setRaw8(1, div);
    ev->setRaw8(2, 24 * 4 / den);
    ev->setRaw8(3, 8);

    TimeSigRec newts;
    newts.bar = bar;
    newts.num = num;
    newts.den = den;
    newts.time = 0;
    if (m_bars.isEmpty()) {
        m_bars.append(newts);
    } else {
        bool found = false;
        foreach(const TimeSigRec& ts, m_bars) {
            if (ts.bar == bar) {
                newts.time = ts.time;
                found = true;
                break;
            }
        }
        if (!found) {
            TimeSigRec& lasts = m_bars.last();
            newts.time = lasts.time +
                    (lasts.num * 4 / lasts.den * m_division * (bar - lasts.bar));
            m_bars.append(newts);
        }
    }
    (this->*m_appendFunc)(newts.time, 0, ev);
}

void SequenceModel::keySigEventWRK(int bar, int alt)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_KEYSIGN);
    ev->setRaw8(0, alt);
    long time = 0;
    foreach(const TimeSigRec& ts, m_bars) {
        if (ts.bar == bar) {
            time = ts.time;
            break;
        }
    }
    (this->*m_appendFunc)(time, 0, ev);
}

void SequenceModel::tempoEvent(long time, int tempo)
{
    double bpm = tempo / 100.0;
    if ( m_initialTempo < 0 )
    {
        m_initialTempo = round( bpm );
    }
    SequencerEvent* ev = new TempoEvent(m_queueId, round ( 6e7 / bpm ) );
    (this->*m_appendFunc)(time, 0, ev);
}

void SequenceModel::trackPatch(int track, int patch)
{
    int channel = 0;
    TrackMapRec rec = m_trackMap[track];
    if (rec.channel > -1)
        channel = rec.channel;
    programEvent(track, 0, channel, patch);
}

void SequenceModel::comments(const QString& cmt)
{
    SequencerEvent* ev = new TextEvent("Comment: " + cmt, 1);
    (this->*m_appendFunc)(0, 0, ev);
}

void SequenceModel::variableRecord(const QString& name, const QByteArray& data)
{
    SequencerEvent* ev = NULL;
    QString s;
    bool isReadable = ( name == "Title" || name == "Author" ||
                       name == "Copyright" || name == "Subtitle" ||
                       name == "Instructions" || name == "Keywords" );
    if (isReadable) {
        QByteArray b2 = data.left(qstrlen(data));
        if (m_wrk->getTextCodec() == 0)
            s = QString(b2);
        else
            s = m_wrk->getTextCodec()->toUnicode(b2);
        if ( name == "Title" )
            ev = new TextEvent(s, 3);
        else if ( name == "Copyright" )
            ev = new TextEvent(s, 2);
        else
            ev = new TextEvent(name + ": " + s, 1);
        (this->*m_appendFunc)(0, 0, ev);
    }
}

void SequenceModel::newTrackHeader( const QString& name,
                              int trackno, int channel,
                              int pitch, int velocity, int /*port*/,
                              bool /*selected*/, bool muted, bool /*loop*/ )
{
    m_currentTrack = trackno;
    TrackMapRec rec;
    rec.channel = channel;
    rec.pitch = pitch;
    rec.velocity = velocity;
    m_trackMap[trackno] = rec;
    m_ntrks++;
    m_tempSong.setMutedState(m_currentTrack, muted);
    if (!name.isEmpty())
        textEvent(trackno, 0, 3, name);
    if (m_reportsFilePos)
        emit loadProgress(m_wrk->getFilePos());
}

void SequenceModel::trackName(int trackno, const QString& name)
{
    SequencerEvent* ev = new TextEvent(name, 3);
    (this->*m_appendFunc)(0, trackno, ev);
}

void SequenceModel::trackVol(int track, int vol)
{
    int channel = 0;
    int lsb, msb;
    TrackMapRec rec = m_trackMap[track];
    if (rec.channel > -1)
        channel = rec.channel;
    if (vol < 128)
        wrkCtlChangeEvent(track, 0, channel, MIDI_CTL_MSB_MAIN_VOLUME, vol);
    else {
        lsb = vol % 0x80;
        msb = vol / 0x80;
        wrkCtlChangeEvent(track, 0, channel, MIDI_CTL_LSB_MAIN_VOLUME, lsb);
        wrkCtlChangeEvent(track, 0, channel, MIDI_CTL_MSB_MAIN_VOLUME, msb);
    }
}

void SequenceModel::trackBank(int track, int bank)
{
    int method = 0;
    int channel = 0;
    int lsb, msb;
    TrackMapRec rec = m_trackMap[track];
    if (rec.channel > -1)
        channel = rec.channel;
    if (channel == 9 && m_ins2 != NULL)
        method = m_ins2->bankSelMethod();
    else if (m_ins != NULL)
        method = m_ins->bankSelMethod();
    switch (method) {
    case 0:
        lsb = bank % 0x80;
        msb = bank / 0x80;
        wrkCtlChangeEvent(track, 0, channel, MIDI_CTL_MSB_BANK, msb);
        wrkCtlChangeEvent(track, 0, channel, MIDI_CTL_LSB_BANK, lsb);
        break;
    case 1:
        wrkCtlChangeEvent(track, 0, channel, MIDI_CTL_MSB_BANK, bank);
        break;
    case 2:
        wrkCtlChangeEvent(track, 0, channel, MIDI_CTL_LSB_BANK, bank);
        break;
    default: /* if method is 3 or above, do nothing */
        break;
    }
    m_lastBank[channel] = bank;
}

void SequenceModel::segment(int track, long time, const QString& name)
{
    if (!name.isEmpty()) {
        SequencerEvent *ev = new TextEvent("Segment: " + name, 6);
        (this->*m_appendFunc)(time, track, ev);
    }
}

void SequenceModel::chord(int track, long time, const QString& name, const QByteArray& /*data*/ )
{
    if (!name.isEmpty()) {
        SequencerEvent *ev = new TextEvent("Chord: " + name, 1);
        (this->*m_appendFunc)(time, track, ev);
    }
}

void SequenceModel::expression(int track, long time, int /*code*/, const QString& text)
{
    if (!text.isEmpty()) {
        SequencerEvent *ev = new TextEvent(text, 1);
        (this->*m_appendFunc)(time, track, ev);
    }
}

void SequenceModel::endOfWrk()
{
    if (m_initialTempo < 0)
        m_initialTempo = TEMPO_BPM;
}

void SequenceModel::unknownChunk(int type, const QByteArray& data)
{
    kDebug() << "dec:" << type
             << "hex:" << hex << type << dec
             << "size:" << data.length();
}

int SequenceModel::getTrackForIndex(int idx)
{
    QList<int> indexes = m_trackMap.keys();
    if (!indexes.isEmpty())
        return indexes.at(idx) + 1;
    else
        return idx+1;
}

/* ********************************* *
 * Overture OVE file format handling
 * ********************************* */

double SequenceModel::oveRealTime(long ticks) const
{
    double division = 1.0 * m_division;
    TempoRec last;
    last.time = 0;
    last.tempo = 100.0;
    last.seconds = 0.0;
    if (!m_tempos.isEmpty()) {
        foreach(const TempoRec& rec, m_tempos) {
            if (rec.time >= ticks)
                break;
            last = rec;
        }
    }
    return last.seconds + (((ticks - last.time) / division) * (60.0 / last.tempo));
}

void
SequenceModel::appendOVEEvent(long ticks, int track, SequencerEvent* ev)
{
    double seconds = oveRealTime(ticks);
    appendEvent(ticks, seconds, track, ev);
    KApplication::processEvents();
}

void SequenceModel::oveErrorHandler(const QString& errorStr)
{
    m_loadingMessages.append(errorStr);
}

void SequenceModel::oveFileHeader(int quarter, int /*trackCount*/)
{
    m_fileFormat = i18n("Overture File");
    m_format = 1;
    m_ntrks = 0; // dynamically calculated
    m_division = quarter;
}

void SequenceModel::oveNoteOnEvent(int track, long tick, int channel, int pitch, int vol)
{
    SequencerEvent* ev = new NoteOnEvent(channel, pitch, vol);
    (this->*m_appendFunc)(tick, track, ev);
}

void SequenceModel::oveNoteOffEvent(int track, long tick, int channel, int pitch, int vol)
{
    SequencerEvent* ev = new NoteOffEvent(channel, pitch, vol);
    (this->*m_appendFunc)(tick, track, ev);
}

void SequenceModel::oveTrackPatch(int track, int channel, int patch)
{
    int ch = channel;
    TrackMapRec rec = m_trackMap[track];
    if (rec.channel > -1)
        ch = rec.channel;
    programEvent(track, 0, ch, patch);
}

void SequenceModel::oveTrackVol(int track, int channel, int vol)
{
    int ch = channel;
    int lsb, msb;
    TrackMapRec rec = m_trackMap[track];
    if (rec.channel > -1)
        ch = rec.channel;
    if (vol < 128)
        wrkCtlChangeEvent(track, 0, ch, MIDI_CTL_MSB_MAIN_VOLUME, vol);
    else {
        lsb = vol % 0x80;
        msb = vol / 0x80;
        wrkCtlChangeEvent(track, 0, ch, MIDI_CTL_LSB_MAIN_VOLUME, lsb);
        wrkCtlChangeEvent(track, 0, ch, MIDI_CTL_MSB_MAIN_VOLUME, msb);
    }
}

void SequenceModel::oveTrackBank(int track, int channel, int bank)
{
    // assume GM/GS bank method
    int ch = channel;
    int lsb, msb;
    TrackMapRec rec = m_trackMap[track];
    if (rec.channel > -1)
        ch = rec.channel;
    lsb = bank % 0x80;
    msb = bank / 0x80;
    wrkCtlChangeEvent(track, 0, ch, MIDI_CTL_MSB_BANK, msb);
    wrkCtlChangeEvent(track, 0, ch, MIDI_CTL_LSB_BANK, lsb);
}

void SequenceModel::oveTextEvent(int track, long tick, const QString& data)
{
    SequencerEvent* ev = new TextEvent(data, 1);
    (this->*m_appendFunc)(tick, track, ev);
}

void SequenceModel::oveTimeSigEvent(int bar, long tick, int num, int den)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_TIMESIGN);
    int div, d = den;
    for ( div = 0; d > 1; d /= 2 )
        ++div;
    ev->setRaw8(0, num);
    ev->setRaw8(1, div);
    ev->setRaw8(2, 24 * 4 / den);
    ev->setRaw8(3, 8);
    TimeSigRec newts;
    newts.bar = bar;
    newts.num = num;
    newts.den = den;
    newts.time = tick;
    m_bars.append(newts);
    (this->*m_appendFunc)(tick, 0, ev);
}

void SequenceModel::oveKeySigEvent(int /*bar*/, long tick, int alt)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_KEYSIGN);
    ev->setRaw8(0, alt);
    (this->*m_appendFunc)(tick, 0, ev);
}

void SequenceModel::oveTempoEvent(long time, int tempo)
{
    double bpm = tempo / 100.0;
    double division = 1.0 * m_division;
    if ( m_initialTempo < 0 ) {
        m_initialTempo = round( bpm );
    }
    TempoRec last, next;
    next.time = time;
    next.tempo = bpm;
    next.seconds = 0.0;
    last.time = 0;
    last.tempo = next.tempo;
    last.seconds = 0.0;
    if (!m_tempos.isEmpty()) {
        foreach(const TempoRec& rec, m_tempos) {
            if (rec.time >= time)
                break;
            last = rec;
        }
        next.seconds = last.seconds +
            (((time - last.time) / division) * (60.0 / last.tempo));
    }
    m_tempos.append(next);
    SequencerEvent* ev = new TempoEvent(m_queueId, round ( 6e7 / bpm ) );
    (this->*m_appendFunc)(time, 0, ev);
}
