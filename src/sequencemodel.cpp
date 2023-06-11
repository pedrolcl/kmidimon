/***************************************************************************
 *   Drumstick MIDI monitor based on the ALSA Sequencer                    *
 *   Copyright (C) 2005-2023 Pedro Lopez-Cabanillas                        *
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

#include <cmath>
#include <QApplication>
#include <QTextCodec>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QListIterator>
#include <QTime>
#include <QTextStream>
#include <QDebug>

#include "sequencemodel.h"
#include "sequenceradaptor.h"
#include "kmidimon.h"
#include "eventfilter.h"

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
#define endl Qt::endl
#define hex Qt::hex
#define dec Qt::dec
#endif

using namespace drumstick;
using namespace ALSA;
using namespace File;

/* Song */

static inline bool eventLessThan(const SequenceItem& s1, const SequenceItem& s2)
{
    return s1.getTicks() < s2.getTicks();
}

void Song::sort()
{
    std::sort(begin(), end(), eventLessThan);
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

Song Song::filtered()
{
    Song result;
    std::copy_if(begin(), end(), std::back_inserter(result), [this](SequenceItem &item) -> bool {
        return !m_mutedState[item.getTrack()];
    });
    result.setLast(getLast());
    return result;
}

/* TextEvent2 is another version of TextEvent, without explicit encoding/decoding */

TextEvent2::TextEvent2()
    : VariableEvent(), m_textType(1)
{
    setSequencerType(SND_SEQ_EVENT_USR_VAR0);
}

/**
 * Constructor from an ALSA sequencer record.
 * @param event ALSA sequencer record.
 */
TextEvent2::TextEvent2(const snd_seq_event_t* event)
    : VariableEvent(event), m_textType(1)
{
    setSequencerType(SND_SEQ_EVENT_USR_VAR0);
}

/**
 * Constructor from a given string
 * @param text The event's text
 * @param textType The SMF text type
 */
TextEvent2::TextEvent2(const QByteArray& text, const int textType)
    : VariableEvent(text), m_textType(textType)
{
    setSequencerType(SND_SEQ_EVENT_USR_VAR0);
}

/**
 * Copy constructor
 * @param other An existing TextEvent2 object reference
 */
TextEvent2::TextEvent2(const TextEvent2& other)
    : VariableEvent(other)
{
    setSequencerType(SND_SEQ_EVENT_USR_VAR0);
    m_textType = other.getTextType();
}

/**
 * Constructor from a data pointer and length
 * @param datalen Data length
 * @param dataptr Data pointer
 */
TextEvent2::TextEvent2(const unsigned int datalen, char* dataptr)
    : VariableEvent(datalen, dataptr), m_textType(1)
{
    setSequencerType(SND_SEQ_EVENT_USR_VAR0);
}

/**
 * Gets the event's SMF text type.
 * @return The SMF text type.
 */
int TextEvent2::getTextType() const
{
    return m_textType;
}

/**
 * Clone this object returning a pointer to the new object
 * @return pointer to the new object
 */
TextEvent2* TextEvent2::clone() const
{
    return new TextEvent2(&m_event);
}

/* EchoEvent is a loopback sequencer event with an int value parameter */

EchoEvent::EchoEvent(): drumstick::ALSA::SequencerEvent()
{
    snd_seq_ev_set_fixed(&m_event);
    setSequencerType(SND_SEQ_EVENT_ECHO);
    setValue(-1);
};

EchoEvent::EchoEvent(const snd_seq_event_t* event): SequencerEvent(event)
{
    snd_seq_ev_set_fixed(&m_event);
    setSequencerType(SND_SEQ_EVENT_ECHO);
    setValue(-1);
};

EchoEvent::EchoEvent(int val) : SequencerEvent()
{
    snd_seq_ev_set_fixed(&m_event);
    setSequencerType(SND_SEQ_EVENT_ECHO);
    setValue(val);
}

EchoEvent* EchoEvent::clone() const
{
    return new EchoEvent(&m_event);
}

/* SequenceModel */

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
        m_ins(nullptr),
        m_ins2(nullptr),
        m_filter(nullptr),
        m_codec(nullptr),
        m_appendFunc(nullptr)
{
    m_rmid = new Rmidi(this);
    connect(m_rmid, &Rmidi::signalRiffData,
                    this, &SequenceModel::dataHandler );
    connect(m_rmid, &Rmidi::signalRiffInfo,
                    this, &SequenceModel::infoHandler );

    m_smf = new QSmf(this);
    connect(m_smf, &QSmf::signalSMFHeader,
                   this, &SequenceModel::headerEvent);
    connect(m_smf, &QSmf::signalSMFTrackStart,
                   this, &SequenceModel::trackStartEvent);
    connect(m_smf, &QSmf::signalSMFTrackEnd,
                   this, &SequenceModel::trackEndEvent);
    connect(m_smf, &QSmf::signalSMFNoteOn,
                   this, &SequenceModel::noteOnEvent);
    connect(m_smf, &QSmf::signalSMFNoteOff,
                   this, &SequenceModel::noteOffEvent);
    connect(m_smf, &QSmf::signalSMFKeyPress,
                   this, &SequenceModel::keyPressEvent);
    connect(m_smf, &QSmf::signalSMFCtlChange,
                   this, &SequenceModel::smfCtlChangeEvent);
    connect(m_smf, &QSmf::signalSMFPitchBend,
                   this, &SequenceModel::pitchBendEvent);
    connect(m_smf, &QSmf::signalSMFProgram,
                   this, &SequenceModel::programEvent);
    connect(m_smf, &QSmf::signalSMFChanPress,
                   this, &SequenceModel::chanPressEvent);
    connect(m_smf, &QSmf::signalSMFSysex,
                   this, &SequenceModel::sysexEvent);
    connect(m_smf, &QSmf::signalSMFMetaUnregistered,
                   this, &SequenceModel::metaMiscEvent);
    connect(m_smf, &QSmf::signalSMFSeqSpecific,
                   this, &SequenceModel::seqSpecificEvent);
    connect(m_smf, &QSmf::signalSMFText2,
                   this, &SequenceModel::textEvent);
    connect(m_smf, &QSmf::signalSMFendOfTrack,
                   this, &SequenceModel::endOfTrackEvent);
    connect(m_smf, &QSmf::signalSMFTempo,
                   this, &SequenceModel::tempoEvent);
    connect(m_smf, &QSmf::signalSMFTimeSig,
                   this, &SequenceModel::timeSigEvent);
    connect(m_smf, &QSmf::signalSMFKeySig,
                   this, &SequenceModel::keySigEventSMF);
    connect(m_smf, &QSmf::signalSMFError,
                   this, &SequenceModel::errorHandlerSMF);
    connect(m_smf, &QSmf::signalSMFWriteTrack,
                   this, &SequenceModel::trackHandler);
    connect(m_smf, &QSmf::signalSMFSequenceNum,
                   this, &SequenceModel::seqNum);
    connect(m_smf, &QSmf::signalSMFforcedChannel,
                   this, &SequenceModel::forcedChannel);
    connect(m_smf, &QSmf::signalSMFforcedPort,
                   this, &SequenceModel::forcedPort);
    connect(m_smf, &QSmf::signalSMFSmpte,
                   this, &SequenceModel::smpteEvent);

    m_wrk = new QWrk(this);
    connect(m_wrk, &QWrk::signalWRKError,
                   this, &SequenceModel::errorHandlerWRK);
    connect(m_wrk, &QWrk::signalWRKUnknownChunk,
                   this, &SequenceModel::unknownChunk);
    connect(m_wrk, &QWrk::signalWRKHeader,
                   this, &SequenceModel::fileHeader);
    connect(m_wrk, &QWrk::signalWRKEnd,
                   this, &SequenceModel::endOfWrk);
    connect(m_wrk, &QWrk::signalWRKStreamEnd,
                   this, &SequenceModel::streamEndEvent);
    connect(m_wrk, &QWrk::signalWRKGlobalVars,
                   this, &SequenceModel::globalVars);
    connect(m_wrk, &QWrk::signalWRKTrack2,
                   this, &SequenceModel::trackHeader);
    connect(m_wrk, &QWrk::signalWRKTimeBase,
                   this, &SequenceModel::timeBase);
    connect(m_wrk, &QWrk::signalWRKNote,
                   this, &SequenceModel::noteEvent);
    connect(m_wrk, &QWrk::signalWRKKeyPress,
                   this, &SequenceModel::keyPressEventWRK);
    connect(m_wrk, &QWrk::signalWRKCtlChange,
                   this, &SequenceModel::wrkCtlChangeEvent);
    connect(m_wrk, &QWrk::signalWRKPitchBend,
                   this, &SequenceModel::pitchBendEventWRK);
    connect(m_wrk, &QWrk::signalWRKProgram,
                   this, &SequenceModel::programEventWRK);
    connect(m_wrk, &QWrk::signalWRKChanPress,
                   this, &SequenceModel::chanPressEventWRK);
    connect(m_wrk, &QWrk::signalWRKSysexEvent,
                   this, &SequenceModel::sysexEventWRK);
    connect(m_wrk, &QWrk::signalWRKSysex,
                   this, &SequenceModel::sysexEventBank);
    connect(m_wrk, &QWrk::signalWRKText2,
                   this, &SequenceModel::textEventWRK);
    connect(m_wrk, &QWrk::signalWRKTimeSig,
                   this, &SequenceModel::timeSigEventWRK);
    connect(m_wrk, &QWrk::signalWRKKeySig,
                   this, &SequenceModel::keySigEventWRK);
    connect(m_wrk, &QWrk::signalWRKTempo,
                   this, &SequenceModel::tempoEventWRK);
    connect(m_wrk, &QWrk::signalWRKTrackPatch,
                   this, &SequenceModel::trackPatch);
    connect(m_wrk, &QWrk::signalWRKComments2,
                   this, &SequenceModel::comments);
    connect(m_wrk, &QWrk::signalWRKVariableRecord,
                   this, &SequenceModel::variableRecord);
    connect(m_wrk, &QWrk::signalWRKNewTrack2,
                   this, &SequenceModel::newTrackHeader);
    connect(m_wrk, &QWrk::signalWRKTrackName2,
                   this, &SequenceModel::trackName);
    connect(m_wrk, &QWrk::signalWRKTrackVol,
                   this, &SequenceModel::trackVol);
    connect(m_wrk, &QWrk::signalWRKTrackBank,
                   this, &SequenceModel::trackBank);
    connect(m_wrk, &QWrk::signalWRKSegment2,
                   this, &SequenceModel::segment);
    connect(m_wrk, &QWrk::signalWRKChord,
                   this, &SequenceModel::chord);
    connect(m_wrk, &QWrk::signalWRKExpression2,
                   this, &SequenceModel::expression);
    connect(m_wrk, &QWrk::signalWRKMarker2,
                   this, &SequenceModel::marker);

    QString data = KMidimon::dataDirectory();
    if (data.isEmpty()) {
        m_insList.load(":/data/standards.ins");
    } else {
        QFileInfo f(data, "standards.ins");
        if (f.exists()) {
            m_insList.load(f.absoluteFilePath());
        } else {
            m_insList.load(":/data/standards.ins");
        }
    }
}

SequenceModel::~SequenceModel()
{
    clear();
}

Qt::ItemFlags
SequenceModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant
SequenceModel::headerData(int section, Qt::Orientation orientation,
                          int role) const
{
    if ((orientation == Qt::Horizontal) && (role == Qt::DisplayRole)) {
        switch(section) {
        case 0:
            return tr("Ticks");
        case 1:
            return tr("Time");
        case 2:
            return tr("Source","event origin");
        case 3:
            return tr("Event kind");
        case 4:
            return tr("Chan");
        case 5:
            return tr("Data 1");
        case 6:
            return tr("Data 2");
        case 7:
            return tr("Data 3");
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
            if (ev != nullptr) {
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
    m_items.setLast(itm.getTicks());
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
    m_infoMap.clear();
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
        if (ev != nullptr) {
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
    if (sev != nullptr) {
        if (m_translateSysex) {
            unsigned char *ptr = (unsigned char *) sev->getData();
            if (sev->getLength() < 6) return QString();
            if (ptr[0] != 0xf0) return QString();
            int msgId = ptr[1];
            switch (msgId) {
            case 0x7e:
                return tr("Universal Non Real Time SysEx");
            case 0x7f:
                return tr("Universal Real Time SysEx");
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
    if (sev != nullptr) {
        if (m_translateSysex) {
            unsigned char *ptr = (unsigned char *) sev->getData();
            if (sev->getLength() < 6) return QString();
            if (ptr[0] != 0xf0) return QString();
            unsigned char deviceId = ptr[2];
            if ( deviceId == 0x7f )
                return tr("broadcast","cast or scattered in all directions");
            else
                return tr("device %1").arg(deviceId);
        }
        return "-";
    }
    return QString();
}

QString
SequenceModel::sysex_data1(const SequencerEvent *ev) const
{
    const SysExEvent *sev = static_cast<const SysExEvent*>(ev);
    if (sev != nullptr) {
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
                        return tr("Sample Dump");
                    case 0x04:
                        return tr("MTC");
                    case 0x05:
                        return tr("Sample Dump");
                    case 0x06:
                        return tr("Gen.Info","General Info");
                    case 0x07:
                        return tr("File Dump");
                    case 0x08:
                        return tr("Tuning");
                    case 0x09:
                        return tr("GM Mode","General MIDI mode");
                    case 0x0a:
                        return tr("DLS","Downloadable Sounds");
                    case 0x0b:
                        return tr("File Ref.","File Reference");
                    case 0x7b:
                        return tr("End of File");
                    case 0x7c:
                        return tr("Wait");
                    case 0x7d:
                        return tr("Cancel");
                    case 0x7e:
                        return tr("NAK");
                    case 0x7f:
                        return tr("ACK");
                    default:
                        break;
                }
            } else
            if (msgId == 0x7f) { // Universal Real Time
                switch (subId1) {
                    case 0x01:
                        return tr("MTC");
                    case 0x02:
                        return tr("Show Control");
                    case 0x03:
                        return tr("Notation");
                    case 0x04:
                        return tr("Device Control");
                    case 0x05:
                        return tr("MTC Cueing");
                    case 0x06:
                        return tr("MMC Command");
                    case 0x07:
                        return tr("MMC Response");
                    case 0x08:
                        return tr("Tuning");
                    case 0x09:
                        return tr("GM2 Destination","General MIDI 2 Controller Destination");
                    case 0x0a:
                        return tr("Instrument","Key-based Instrument Control");
                    case 0x0b:
                        return tr("Polyphony","Scalable Polyphony MIDI MIP Message");
                    case 0x0c:
                        return tr("Mobile Phone","Mobile Phone Control Message");
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
            return tr("Special","MTC special setup");
        case 0x01:
            return tr("Punch In Points");
        case 0x02:
            return tr("Punch Out Points");
        case 0x03:
            return tr("Delete Punch In Points");
        case 0x04:
            return tr("Delete Punch Out Points");
        case 0x05:
            return tr("Event Start Point");
        case 0x06:
            return tr("Event Stop Point");
        case 0x07:
            return tr("Event Start Point With Info");
        case 0x08:
            return tr("Event Stop Point With Info");
        case 0x09:
            return tr("Delete Event Start Point");
        case 0x0a:
            return tr("Delete Event Stop Point");
        case 0x0b:
            return tr("Cue Points");
        case 0x0c:
            return tr("Cue Points With Info");
        case 0x0d:
            return tr("Delete Cue Point");
        case 0x0e:
            return tr("Event Name");
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
        return tr("Stop");
    case 0x02:
        return tr("Play");
    case 0x03:
        return tr("Deferred play");
    case 0x04:
        return tr("Fast forward");
    case 0x05:
        return tr("Rewind");
    case 0x06:
        return tr("Punch in");
    case 0x07:
        return tr("Punch out");
    case 0x08:
        return tr("Pause recording");
    case 0x09:
        return tr("Pause");
    case 0x0a:
        return tr("Eject");
    case 0x0b:
        return tr("Chase");
    case 0x0c:
        return tr("Error reset");
    case 0x0d:
        return tr("Reset");
    case 0x40:
        return tr("Write");
    case 0x41:
        return tr("Masked Write");
    case 0x42:
        return tr("Read");
    case 0x43:
        return tr("Update");
    case 0x44:
        return tr("Locate");
    case 0x45:
        return tr("Variable play");
    case 0x46:
        return tr("Search");
    case 0x47:
        return tr("Shuttle");
    case 0x48:
        return tr("Step");
    default:
        break;
    }
    return QString();
}

QString
SequenceModel::sysex_data2(const SequencerEvent *ev) const
{
    const SysExEvent *sev = static_cast<const SysExEvent*>(ev);
    if (sev != nullptr) {
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
                        return tr("Header");
                    case 0x02:
                        return tr("Data Packet");
                    case 0x03:
                        return tr("Request");
                    case 0x04:
                        return sysex_mtc(subId2);
                        break;
                    case 0x05:
                        switch (subId2) {
                            case 0x01:
                                return tr("Loop Points Send");
                            case 0x02:
                                return tr("Loop Points Request");
                            case 0x03:
                                return tr("Sample Name Send");
                            case 0x04:
                                return tr("Sample Name Request");
                            case 0x05:
                                return tr("Ext.Dump Header");
                            case 0x06:
                                return tr("Ext.Loop Points Send");
                            case 0x07:
                                return tr("Ext.Loop Points Request");
                            default:
                                break;
                        }
                        break;
                    case 0x06:
                        switch (subId2) {
                            case 0x01:
                                return tr("Identity Request");
                            case 0x02:
                                return tr("Identity Reply");
                            default:
                                break;
                        }
                        break;
                    case 0x07:
                        switch (subId2) {
                            case 0x01:
                                return tr("Header");
                            case 0x02:
                                return tr("Data Packet");
                            case 0x03:
                                return tr("Request");
                            default:
                                break;
                        }
                        break;
                    case 0x08:
                        switch (subId2) {
                            case 0x00:
                                return tr("Dump Request");
                            case 0x01:
                                return tr("Bulk Dump");
                            case 0x02:
                                return tr("Note Change");
                            case 0x03:
                                return tr("Tuning Dump Request");
                            case 0x04:
                                return tr("Key-based Tuning Dump");
                            case 0x05:
                                return tr("Scale/Octave Dump 1b");
                            case 0x06:
                                return tr("Scale/Octave Dump 2b");
                            case 0x07:
                                return tr("Single Note Change");
                            case 0x08:
                                return tr("Scale/Octave Tuning 1b");
                            case 0x09:
                                return tr("Scale/Octave Tuning 2b");
                            default:
                                break;
                        }
                        break;
                    case 0x09:
                        switch (subId2) {
                            case 0x01:
                                return tr("GM On");
                            case 0x02:
                                return tr("GM Off");
                            case 0x03:
                                return tr("GM2 On");
                            default:
                                break;
                        }
                        break;
                    case 0x0a:
                        switch (subId2) {
                            case 0x01:
                                return tr("DLS On");
                            case 0x02:
                                return tr("DLS Off");
                            case 0x03:
                                return tr("DLS Voice Alloc. Off");
                            case 0x04:
                                return tr("DLS Voice Alloc. On");
                            default:
                                break;
                        }
                        break;
                    case 0x0b:
                        switch (subId2) {
                            case 0x01:
                                return tr("Open");
                            case 0x02:
                                return tr("Select Contents");
                            case 0x03:
                                return tr("Open and Select");
                            case 0x04:
                                return tr("Close");
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
                                return tr("Full Frame");
                            case 0x02:
                                return tr("User Bits");
                            default:
                                break;
                        }
                        break;
                    case 0x02:
                        switch (subId2) {
                            case 0x00:
                                return tr("MSC Extension");
                            default:
                                return tr("MSC Cmd.%1").arg(subId2);
                        }
                        break;
                    case 0x03:
                        switch (subId2) {
                            case 0x01:
                                return tr("Bar Marker");
                            case 0x02:
                            case 0x42:
                                return tr("Time Signature");
                            default:
                                break;
                        }
                        break;
                    case 0x04:
                        switch (subId2) {
                            case 0x01:
                                return tr("Volume","sound volume");
                            case 0x02:
                                return tr("Balance","sound balance");
                            case 0x03:
                                return tr("Fine Tuning");
                            case 0x04:
                                return tr("Coarse Tuning");
                            case 0x05:
                                return tr("Global Parameter");
                            default:
                                break;
                        }
                        break;
                    case 0x05:
                        return sysex_mtc(subId2);
                    case 0x06:
                        return sysex_mmc(subId2);
                    case 0x07:
                        return tr("Response %1").arg(subId2);
                    case 0x08:
                        switch (subId2) {
                            case 0x02:
                                return tr("Single Note");
                            case 0x07:
                                return tr("Single Note with Bank");
                            case 0x08:
                                return tr("Scale/Octave 1b");
                            case 0x09:
                                return tr("Scale/Octave 2b");
                            default:
                                break;
                        }
                        break;
                    case 0x09:
                        switch (subId2) {
                            case 0x01:
                                return tr("Channel aftertouch");
                            case 0x02:
                                return tr("Polyphonic aftertouch");
                            case 0x03:
                                return tr("Controller");
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
    if (sev != nullptr) {
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
    if (sev != nullptr) {
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
    if (pe != nullptr)
        return QString("%1:%2").arg(client_name(pe->getClient()))
                               .arg(pe->getPort());
    else
        return QString();
}

QString
SequenceModel::event_client(const SequencerEvent *ev) const
{
    const ClientEvent* ce = static_cast<const ClientEvent*>(ev);
    if (ce != nullptr)
        return client_name(ce->getClient());
    else
        return QString();
}

QString
SequenceModel::event_sender(const SequencerEvent *ev) const
{
    const SubscriptionEvent* se = static_cast<const SubscriptionEvent*>(ev);
    if (se != nullptr)
        return QString("%1:%2").arg(client_name(se->getSenderClient()))
                               .arg(se->getSenderPort());
    else
        return QString();
}

QString
SequenceModel::event_dest(const SequencerEvent *ev) const
{
    const SubscriptionEvent* se = static_cast<const SubscriptionEvent*>(ev);
    if (se != nullptr)
        return QString("%1:%2").arg(client_name(se->getDestClient()))
                               .arg(se->getDestPort());
    else
        return QString();
}

QString
SequenceModel::common_param(const SequencerEvent *ev) const
{
    const ValueEvent* ve = static_cast<const ValueEvent*>(ev);
    if (ve != nullptr)
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
    if (m_filter != nullptr)
        res = m_filter->getName(ev->getSequencerType());
    if (res.isEmpty())
        res = tr("Event type %1").arg(ev->getSequencerType());
    return res;
}

QString
SequenceModel::event_channel(const SequencerEvent *ev) const
{
    if (SequencerEvent::isChannel(ev)) {
        const ChannelEvent* che = static_cast<const ChannelEvent*>(ev);
        if (che != nullptr)
            return QString::number(che->getChannel()+1);
    } else
        if (ev->getSequencerType() == SND_SEQ_EVENT_SYSEX)
            return sysex_chan(ev);
    return QString();
}

QString
SequenceModel::note_name(const int note) const
{
    const QString m_names_s[] = {tr("C"), tr("C♯"), tr("D"), tr("D♯"), tr("E"),
    		tr("F"), tr("F♯"), tr("G"), tr("G♯"), tr("A"), tr("A♯"), tr("B")};
    const QString m_names_f[] = {tr("C"), tr("D♭"), tr("D"), tr("E♭"), tr("E"),
    		tr("F"), tr("G♭"), tr("G"), tr("A♭"), tr("A"), tr("B♭"), tr("B")};
    int num = note % 12;
    int oct = (note / 12) - 1;
    QString name = m_useFlats ? m_names_f[num] : m_names_s[num];
    return QString("%1%2:%3").arg(name).arg(oct).arg(note);
}

QString
SequenceModel::note_key(const SequencerEvent* ev) const
{
    const KeyEvent* ke = static_cast<const KeyEvent*>(ev);
    if (ke != nullptr)
    	if (m_translateNotes)
            if ((ke->getChannel() == MIDI_GM_DRUM_CHANNEL) && (m_ins2 != nullptr)) {
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
    if (pc != nullptr) {
        if (m_translateCtrls) {
            if (pc->getChannel() == MIDI_GM_DRUM_CHANNEL && m_ins2 != nullptr) {
                const InstrumentData& patch = m_ins2->patch(m_lastBank[pc->getChannel()]);
                if (patch.contains(pc->getValue()))
                    return QString("%1:%2").arg(patch[pc->getValue()]).arg(pc->getValue());
            }
            if (pc->getChannel() != MIDI_GM_DRUM_CHANNEL && m_ins != nullptr) {
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
    if (ke != nullptr)
        return QString::number(ke->getVelocity());
    else
        return QString();
}

QString
SequenceModel::note_duration(const SequencerEvent* ev) const
{
    const NoteEvent* ne = static_cast<const NoteEvent*>(ev);
    if (ne != nullptr)
        return QString::number(ne->getDuration());
    else
        return QString();
}

QString
SequenceModel::control_param(const SequencerEvent* ev) const
{
    const ControllerEvent* ce = static_cast<const ControllerEvent*>(ev);
    if (ce != nullptr) {
        if (m_translateCtrls) {
            Instrument* ins = nullptr;
            if (ce->getChannel() == MIDI_GM_DRUM_CHANNEL && m_ins2 != nullptr)
                ins = m_ins2;
            if (m_ins != nullptr)
                ins = m_ins;
            if (ins != nullptr) {
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
    if (ce != nullptr)
        return QString::number(ce->getValue());
    else
        return QString();
}

QString
SequenceModel::pitchbend_value(const SequencerEvent* ev) const
{
    const PitchBendEvent* pe = static_cast<const PitchBendEvent*>(ev);
    if (pe != nullptr)
        return QString::number(pe->getValue());
    else
        return QString();
}

QString
SequenceModel::chanpress_value(const SequencerEvent* ev) const
{
    const ChanPressEvent* cp = static_cast<const ChanPressEvent*>(ev);
    if (cp != nullptr)
        return QString::number(cp->getValue());
    else
        return QString();
}

QString
SequenceModel::tempo_bpm(const SequencerEvent *ev) const
{
    const TempoEvent* te = static_cast<const TempoEvent*>(ev);
    if (te != nullptr)
        return tr("%1 bpm").arg(QString::number(6e7 / te->getValue(), 'f', 1));
    else
        return QString();
}

QString
SequenceModel::tempo_npt(const SequencerEvent *ev) const
{
    const TempoEvent* te = static_cast<const TempoEvent*>(ev);
    if (te != nullptr)
        return QString::number(te->getValue());
    else
        return QString();
}

QString
SequenceModel::text_type(const SequencerEvent *ev) const
{
    const TextEvent2* te = static_cast<const TextEvent2*>(ev);
    if (te != nullptr) {
        switch ( te->getTextType() ) {
        case 1:
            return tr("Text:1");
        case 2:
            return tr("Copyright:2");
        case 3:
            return tr("Name:3","song or track name");
        case 4:
            return tr("Instrument:4");
        case 5:
            return tr("Lyric:5");
        case 6:
            return tr("Marker:6");
        case 7:
            return tr("Cue:7");
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
    const TextEvent2* te = static_cast<const TextEvent2*>(ev);
    QByteArray data;
    if (te != nullptr) {
        data = QByteArray(te->getData(), te->getLength());
    }
    if (m_codec != nullptr) {
        return m_codec->toUnicode(data);
    } else {
        return QString::fromLatin1(data);
    }
}

QString
SequenceModel::time_sig1(const SequencerEvent *ev) const
{
    return tr("%1/%2").arg(ev->getRaw8(0)).arg(pow(2, ev->getRaw8(1)));
}

QString
SequenceModel::time_sig2(const SequencerEvent *ev) const
{
    return tr("%1 clocks per click, %2 32nd per quarter").arg(
            ev->getRaw8(2)).arg(
            ev->getRaw8(3) );
}

QString
SequenceModel::key_sig1(const SequencerEvent *ev) const
{
    signed char s = (signed char) ev->getRaw8(0);
    return tr("%1%2").arg(abs(s)).arg(
            s < 0 ? QChar(0x266D) : QChar(0x266F)); //s < 0 ? "♭" : "♯"
}

QString
SequenceModel::key_sig2(const SequencerEvent *ev) const
{
    static QString tmaj[] = {tr("C flat"), tr("G flat"), tr("D flat"),
            tr("A flat"), tr("E flat"), tr("B flat"),tr("F"),
            tr("C"), tr("G"), tr("D"), tr("A"),
            tr("E"), tr("B"), tr("F sharp"),tr("C sharp")};
    static QString tmin[] = {tr("A flat"), tr("E flat"), tr("B flat"),
            tr("F"), tr("C"), tr("G"), tr("D"),
            tr("A"), tr("E"), tr("B"), tr("F sharp"), tr("C sharp"),
            tr("G sharp"), tr("D sharp"), tr("A sharp")};
    signed char s = (signed char) ev->getRaw8(0);
    QString tone, mode;
    if (abs(s) < 8) {
        tone = ( ev->getRaw8(1) == 0 ? tmaj[s + 7] : tmin[s + 7] );
        mode = ( ev->getRaw8(1) == 0 ? tr("major","major mode scale") :
                                       tr("minor","minor mode scale"));
    }
    return tone + ' ' + mode;
}

QString
SequenceModel::smpte(const SequencerEvent *ev) const
{
    return tr( "%1:%2:%3:%4:%5").arg(
                 ev->getRaw8(0)).arg(
                 ev->getRaw8(1)).arg(
                 ev->getRaw8(2)).arg(
                 ev->getRaw8(3)).arg(
                 ev->getRaw8(4));
}

QString
SequenceModel::var_event(const SequencerEvent *ev) const
{
    const VariableEvent* ve = static_cast<const VariableEvent*>(ev);
    if (ve != nullptr) {
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
    if (ve != nullptr) {
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
    return nullptr;
}

const SequencerEvent*
SequenceModel::getEvent(const int row) const
{
    if (!m_items.isEmpty() && (row >= 0) && (row < m_items.size()))
        return m_items[row].getEvent();
    return nullptr;
}

/*QString SequenceModel::dumpItem(const int row) const
{
    Q_UNUSED(row)
    QString evstr;
    QTextStream str(&evstr);
    const SequencerEvent *ev = getEvent(row);
    if (ev != nullptr) {
        str << event_ticks(ev).trimmed() << ","
            << event_source(ev).trimmed() << ","
            << event_channel(ev).trimmed() << ","
            << event_kind(ev).trimmed() << ","
            << event_data1(ev).trimmed() << ","
            << event_data2(ev).trimmed() << ","
            << event_data3(ev).trimmed();
    }
    return evstr;
}*/

void
SequenceModel::loadFromFile(const QString& path)
{
    clear();
    m_tempSong.clear();
    m_currentTrack = -1;
    m_initialTempo = -1;
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(path);
    QFileInfo finfo(path);
    QString ext = finfo.suffix().toLower();
    if (type.name() == "audio/midi" || type.name() == "audio/x-midi" || ext == "mid" || ext == "midi") {
        m_appendFunc = &SequenceModel::appendSMFEvent;
        m_reportsFilePos = true;
        m_smf->readFromFile(path);
    } else if (type.name() == "application/x-riff" || ext == "rmi") {
        m_appendFunc = &SequenceModel::appendSMFEvent;
        m_reportsFilePos = true;
        m_rmid->readFromFile(path);
    } else if (type.name() == "audio/cakewalk" || ext == "wrk") {
        m_appendFunc = &SequenceModel::appendWRKEvent;
        m_reportsFilePos = true;
        m_wrk->readFromFile(path);
    } else {
        m_appendFunc = nullptr;
        m_reportsFilePos = false;
        qWarning() << "unrecognized format:" << type.name();
        return;
    }
    m_tempSong.sort();
    beginInsertRows(QModelIndex(), 0, m_tempSong.count() - 1);
    m_items += m_tempSong;
    endInsertRows();
    m_items.setLast(m_tempSong.getLast());
    m_tempSong.clear();
    if (m_initialTempo < 0) {
        m_initialTempo = TEMPO_BPM;
    }
}

void
SequenceModel::processItems()
{
    m_tempSong.clear();
    // include always a tempo event in the saved MIDI song
    auto res_tempo = std::find_if(m_items.constBegin(), m_items.constEnd(), [](SequenceItem itm) {
        return (itm.getEvent()->getSequencerType() == SND_SEQ_EVENT_TEMPO);
    });
    if (res_tempo == m_items.constEnd()) {
        SequenceItem itm(0, 0, 0, new TempoEvent(m_queueId, round(6e7 / m_initialTempo)));
        m_items.append(itm);
    }
    foreach ( const SequenceItem& itm, m_items ) {
        SequencerEvent* ev = itm.getEvent();
        double seconds = 0;
        long ticks = itm.getTicks();
        int track = itm.getTrack();
        if ( ev != nullptr && ev->getSequencerType() == SND_SEQ_EVENT_NOTE ) {
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
    m_tempSong.setLast(m_items.getLast());
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
    QApplication::processEvents();
}

void SequenceModel::headerEvent(int format, int /*ntrks*/, int division)
{
    m_format = format;
    m_ntrks = 0; // ntrks may be incorrect;
    m_division = division;
    m_fileFormat = tr("SMF type %1").arg(format);
}

void
SequenceModel::trackStartEvent()
{
    m_ntrks++;
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
        if (chan == 9 && m_ins2 != nullptr)
            bsm = m_ins2->bankSelMethod();
        else if (m_ins != nullptr)
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

QString SequenceModel::getMetadataInfo() const
{
    if (!m_infoMap.empty()) {
        QString metadata;
        QMap<QString,QString>::const_iterator i;
        for(i = m_infoMap.constBegin(); i != m_infoMap.constEnd(); ++i) {
            metadata += i.key() + ": <b>" + i.value() + "</b><br/>";
        }
        return metadata;
    }
    return QString();
}

void SequenceModel::dataHandler(const QString &dataType, const QByteArray &data)
{
    if (dataType == "RMID") {
        QDataStream ds(data);
        m_smf->readFromStream(&ds);
        m_fileFormat += tr(" in RIFF container of type %1").arg(dataType);
    }
}

void SequenceModel::infoHandler(const QString &infoType, const QByteArray &data)
{
    const QMap<QString,QString> keyMap{
        {"IALB", "Album"},
        {"IARL", "Archival Location"},
        {"IART", "Artist"},
        {"ICMS", "Commissioned"},
        {"ICMT", "Comments"},
        {"ICOP", "Copyright"},
        {"ICRD", "Creation date"},
        {"IENG", "Engineer"},
        {"IGNR", "Genre"},
        {"IKEY", "Keywords"},
        {"IMED", "Medium"},
        {"INAM", "Name"},
        {"IPRD", "Product"},
        {"ISBJ", "Subject"},
        {"ISFT", "Software"},
        {"ISRC", "Source"},
        {"ISR", "Source Form"},
        {"ITCH", "Technician"}};
    QString key;
    if (keyMap.contains(infoType)) {
        key = keyMap[infoType];
    } else {
        key = infoType;
    }
    QString value;
    if ((m_encoding == "latin1") || (m_codec == nullptr)) {
        value = QString::fromLatin1(data);
    } else {
        value = m_codec->toUnicode(data);
    }
    m_infoMap[key] = value;
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
SequenceModel::textEvent(int type, const QByteArray& data)
{
    SequencerEvent* ev = new TextEvent2(data, type);
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
    if (m_reportsFilePos) {
        m_loadingMessages.append(tr(" at offset %1").arg(m_smf->getFilePos()));
    }
    m_loadingMessages.append("<br/>");
}

void
SequenceModel::trackHandler(int track)
{
    unsigned int delta, last_tick = 0;
    foreach ( const SequenceItem& itm, m_tempSong ) {
        if (itm.getTrack() == track) {
            const SequencerEvent* ev = itm.getEvent();
            if (ev != nullptr) {
                delta = ev->getTick() - last_tick;
                last_tick = ev->getTick();
                switch(ev->getSequencerType()) {
                    case SND_SEQ_EVENT_TEMPO: {
                        const TempoEvent* e = static_cast<const TempoEvent*>(ev);
                        if (e != nullptr)
                            m_smf->writeTempo(delta, e->getValue());
                    }
                    break;
                    case SND_SEQ_EVENT_USR_VAR0: {
                        const TextEvent2* e = static_cast<const TextEvent2*>(ev);
                        if (e != nullptr) {
                            QByteArray b(e->getData(), e->getLength());
                            m_smf->writeMetaEvent(delta, e->getTextType(), b);
                        }
                    }
                    break;
                    case SND_SEQ_EVENT_USR_VAR1: {
                        const VariableEvent* e = static_cast<const VariableEvent*>(ev);
                        if (e != nullptr) {
                            QByteArray b(e->getData(), e->getLength());
                            m_smf->writeMetaEvent(delta, sequencer_specific, b);
                        }
                    }
                    break;
                    case SND_SEQ_EVENT_USR_VAR2: {
                        const VariableEvent* e = static_cast<const VariableEvent*>(ev);
                        if (e != nullptr) {
                            QByteArray b(e->getData(), e->getLength());
                            int type = b.at(0);
                            b.remove(0, 1);
                            m_smf->writeMetaEvent(delta, type, b);
                        }
                    }
                    break;
                    case SND_SEQ_EVENT_SYSEX: {
                        const SysExEvent* e = static_cast<const SysExEvent*>(ev);
                        if (e != nullptr)
                            m_smf->writeMidiEvent(delta, system_exclusive,
                                    (long) e->getLength(),
                                    (char *) e->getData());
                    }
                    break;
                    case SND_SEQ_EVENT_NOTEON: {
                        const KeyEvent* e = static_cast<const KeyEvent*>(ev);
                        if (e != nullptr)
                            m_smf->writeMidiEvent(delta, note_on,
                                    e->getChannel(),
                                    e->getKey(),
                                    e->getVelocity());
                    }
                    break;
                    case SND_SEQ_EVENT_NOTEOFF: {
                        const KeyEvent* e = static_cast<const KeyEvent*>(ev);
                        if (e != nullptr)
                            m_smf->writeMidiEvent(delta, note_off,
                                    e->getChannel(),
                                    e->getKey(),
                                    e->getVelocity());
                    }
                    break;
                    case SND_SEQ_EVENT_KEYPRESS: {
                        const KeyEvent* e = static_cast<const KeyEvent*>(ev);
                        if (e != nullptr)
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
                        if (e != nullptr)
                            m_smf->writeMidiEvent(delta, control_change,
                                    e->getChannel(),
                                    e->getParam(),
                                    e->getValue());
                    }
                    break;
                    case SND_SEQ_EVENT_PGMCHANGE: {
                        const ProgramChangeEvent* e = static_cast<const ProgramChangeEvent*>(ev);
                        if (e != nullptr)
                            m_smf->writeMidiEvent(delta, program_chng,
                                    e->getChannel(), e->getValue());
                    }
                    break;
                    case SND_SEQ_EVENT_CHANPRESS: {
                        const ChanPressEvent* e = static_cast<const ChanPressEvent*>(ev);
                        if (e != nullptr)
                            m_smf->writeMidiEvent(delta, channel_aftertouch,
                                    e->getChannel(), e->getValue());
                    }
                    break;
                    case SND_SEQ_EVENT_PITCHBEND: {
                        const PitchBendEvent* e = static_cast<const PitchBendEvent*>(ev);
                        if (e != nullptr)
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
                    break;
                    case SND_SEQ_EVENT_USR3: {
                        int data = ev->getRaw8(0);
                        m_smf->writeMetaEvent(delta, forced_port, data);
                    }
                    break;
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
    delta = 0;
    if (m_tempSong.getLast() > last_tick) {
        delta = m_tempSong.getLast() - last_tick;
    }
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
        m_ins = nullptr;

    if (m_insList.contains(drmName))
        m_ins2 = &m_insList[drmName];
    else
        m_ins2 = nullptr;
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
    //qDebug() << Q_FUNC_INFO << encoding;
    if (m_encoding != encoding) {
        if (encoding.isEmpty()) {
            m_encoding = QLatin1String("latin1");
        } else {
            m_encoding = encoding;
        }
        m_codec = QTextCodec::codecForName(m_encoding.toLatin1());
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
    QApplication::processEvents();
}

void SequenceModel::errorHandlerWRK(const QString& errorStr)
{
    m_loadingMessages.append(errorStr);
    if (m_reportsFilePos) {
        m_loadingMessages.append(tr(" at offset %1").arg(m_wrk->getFilePos()));
    }
    m_loadingMessages.append("<br/>");
}

void SequenceModel::fileHeader(int verh, int verl)
{
    m_fileFormat = tr("WRK file version %1.%2").arg(verh).arg(verl);
    m_format = 1;
    m_ntrks = 1;
    m_division = 120;
    m_currentTrack = 0;
    TrackMapRec rec;
    m_trackMap[m_currentTrack] = rec;
}

void SequenceModel::timeBase(int timebase)
{
    m_division = timebase;
}

void SequenceModel::globalVars()
{
    keySigEventWRK(0, m_wrk->getKeySig());
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

void SequenceModel::trackHeader( const QByteArray& name1, const QByteArray& name2,
                           int trackno, int channel,
                           int pitch, int velocity, int /*port*/,
                           bool /*selected*/, bool muted, bool /*loop*/ )
{
    m_currentTrack = trackno + 1;
    TrackMapRec rec;
    rec.channel = channel;
    rec.pitch = pitch;
    rec.velocity = velocity;
    m_trackMap[trackno + 1] = rec;
    m_ntrks++;
    m_tempSong.setMutedState(m_currentTrack, muted);
    QByteArray trkName = name1 + ' ' + name2;
    trkName = trkName.trimmed();
    if (!trkName.isEmpty()) {
        SequencerEvent* ev = new TextEvent2(trkName, 3);
        (this->*m_appendFunc)(0, trackno + 1, ev);
    }
    if (m_reportsFilePos)
        emit loadProgress(m_wrk->getFilePos());
}
void SequenceModel::noteEvent(int track, long time, int chan, int pitch, int vol, int dur)
{
    int channel = chan;
    TrackMapRec rec = m_trackMap[track+1];
    int key = pitch + rec.pitch;
    int velocity = vol + rec.velocity;
    if (rec.channel > -1)
        channel = rec.channel;
    SequencerEvent* ev = new NoteEvent(channel, key, velocity, dur);
    (this->*m_appendFunc)(time, track+1, ev);
}

void SequenceModel::keyPressEventWRK(int track, long time, int chan, int pitch, int press)
{
    int channel = chan;
    TrackMapRec rec = m_trackMap[track+1];
    int key = pitch + rec.pitch;
    if (rec.channel > -1)
        channel = rec.channel;
    SequencerEvent* ev = new KeyPressEvent(channel, key, press);
    (this->*m_appendFunc)(time, track+1, ev);
}

void SequenceModel::wrkCtlChangeEvent(int track, long time, int chan, int ctl, int value)
{
    int channel = chan;
    TrackMapRec rec = m_trackMap[track+1];
    if (rec.channel > -1)
        channel = rec.channel;
    ctlChangeEvent(channel, ctl, value);
    SequencerEvent* ev = new ControllerEvent(channel, ctl, value);
    (this->*m_appendFunc)(time, track+1, ev);
}

void SequenceModel::pitchBendEventWRK(int track, long time, int chan, int value)
{
    int channel = chan;
    TrackMapRec rec = m_trackMap[track+1];
    if (rec.channel > -1)
        channel = rec.channel;
    SequencerEvent* ev = new PitchBendEvent(channel, value);
    (this->*m_appendFunc)(time, track+1, ev);
}

void SequenceModel::programEventWRK(int track, long time, int chan, int patch)
{
    int channel = chan;
    TrackMapRec rec = m_trackMap[track+1];
    if (rec.channel > -1)
        channel = rec.channel;
    SequencerEvent* ev = new ProgramChangeEvent(channel, patch);
    (this->*m_appendFunc)(time, track+1, ev);
}

void SequenceModel::chanPressEventWRK(int track, long time, int chan, int press)
{
    int channel = chan;
    TrackMapRec rec = m_trackMap[track+1];
    if (rec.channel > -1)
        channel = rec.channel;
    SequencerEvent* ev = new ChanPressEvent(channel, press);
    (this->*m_appendFunc)(time, track+1, ev);
}

void SequenceModel::sysexEventWRK(int track, long time, int bank)
{
    if (m_savedSysexEvents.contains(bank)) {
        SysExEvent *ev = m_savedSysexEvents[bank].clone();
        (this->*m_appendFunc)(time, track+1, ev);
    }
}

void SequenceModel::sysexEventBank(int bank, const QString& /*name*/, bool autosend, int /*port*/, const QByteArray& data)
{
    SysExEvent ev(data);
    if (autosend) {
        (this->*m_appendFunc)(0, 0, ev.clone());
    } else {
        m_savedSysexEvents[bank] = ev;
    }
}

void SequenceModel::textEventWRK(int track, long time, int /*type*/, const QByteArray& data)
{
    SequencerEvent* ev = new TextEvent2(data, 1);
    (this->*m_appendFunc)(time, track+1, ev);
}

void SequenceModel::timeSigEventWRK(int bar, int num, int den)
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
                    (lasts.num * 4 * m_division / lasts.den * (bar - lasts.bar));
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

void SequenceModel::tempoEventWRK(long time, int tempo)
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
    TrackMapRec rec = m_trackMap[track+1];
    if (rec.channel > -1)
        channel = rec.channel;
    programEventWRK(track+1, 0, channel, patch);
}

void SequenceModel::comments(const QByteArray& cmt)
{
    SequencerEvent* ev = new TextEvent2("Comment: " + cmt, 1);
    (this->*m_appendFunc)(0, 0, ev);
}

void SequenceModel::variableRecord(const QString& name, const QByteArray& data)
{
    SequencerEvent* ev = nullptr;
    //QString s;
    bool isReadable = ( name == "Title"        || name == "Author"   ||
                        name == "Copyright"    || name == "Subtitle" ||
                        name == "Instructions" || name == "Keywords" );
    if (isReadable) {
        QByteArray b2 = data.left(qstrlen(data));
        //if (m_codec == nullptr)
        //   s = QString(b2);
        //else
        //   s = m_codec->toUnicode(b2);
        if ( name == "Title" )
            ev = new TextEvent2(b2, 3);
        else if ( name == "Copyright" )
            ev = new TextEvent2(b2, 2);
        else
            ev = new TextEvent2(name.toLatin1() + ": " + b2, 1);
        (this->*m_appendFunc)(0, 0, ev);
    }
}

void SequenceModel::newTrackHeader( const QByteArray& name,
                              int trackno, int channel,
                              int pitch, int velocity, int /*port*/,
                              bool /*selected*/, bool muted, bool /*loop*/ )
{
    m_currentTrack = trackno + 1;
    TrackMapRec rec;
    rec.channel = channel;
    rec.pitch = pitch;
    rec.velocity = velocity;
    m_trackMap[trackno + 1] = rec;
    m_ntrks++;
    m_tempSong.setMutedState(m_currentTrack, muted);
    if (!name.isEmpty())
        textEventWRK(trackno + 1, 0, 3, name);
    if (m_reportsFilePos)
        emit loadProgress(m_wrk->getFilePos());
}

void SequenceModel::trackName(int trackno, const QByteArray& name)
{
    SequencerEvent* ev = new TextEvent2(name, 3);
    (this->*m_appendFunc)(0, trackno + 1, ev);
}

void SequenceModel::trackVol(int track, int vol)
{
    int channel = 0;
    int lsb, msb;
    TrackMapRec rec = m_trackMap[track+1];
    if (rec.channel > -1)
        channel = rec.channel;
    if (vol < 128)
        wrkCtlChangeEvent(track+1, 0, channel, MIDI_CTL_MSB_MAIN_VOLUME, vol);
    else {
        lsb = vol % 0x80;
        msb = vol / 0x80;
        wrkCtlChangeEvent(track+1, 0, channel, MIDI_CTL_LSB_MAIN_VOLUME, lsb);
        wrkCtlChangeEvent(track+1, 0, channel, MIDI_CTL_MSB_MAIN_VOLUME, msb);
    }
}

void SequenceModel::trackBank(int track, int bank)
{
    int method = 0;
    int channel = 0;
    int lsb, msb;
    TrackMapRec rec = m_trackMap[track+1];
    if (rec.channel > -1)
        channel = rec.channel;
    if (channel == 9 && m_ins2 != nullptr)
        method = m_ins2->bankSelMethod();
    else if (m_ins != nullptr)
        method = m_ins->bankSelMethod();
    switch (method) {
    case 0:
        lsb = bank % 0x80;
        msb = bank / 0x80;
        wrkCtlChangeEvent(track+1, 0, channel, MIDI_CTL_MSB_BANK, msb);
        wrkCtlChangeEvent(track+1, 0, channel, MIDI_CTL_LSB_BANK, lsb);
        break;
    case 1:
        wrkCtlChangeEvent(track+1, 0, channel, MIDI_CTL_MSB_BANK, bank);
        break;
    case 2:
        wrkCtlChangeEvent(track+1, 0, channel, MIDI_CTL_LSB_BANK, bank);
        break;
    default: /* if method is 3 or above, do nothing */
        break;
    }
    m_lastBank[channel] = bank;
}

void SequenceModel::segment(int track, long time, const QByteArray& name)
{
    if (!name.isEmpty()) {
        SequencerEvent *ev = new TextEvent2("Segment: " + name, 6);
        (this->*m_appendFunc)(time, track+1, ev);
    }
}

void SequenceModel::chord(int track, long time, const QString& name, const QByteArray& /*data*/ )
{
    if (!name.isEmpty()) {
        QByteArray ba("Chord: " + name.toLatin1());
        SequencerEvent *ev = new TextEvent2(ba, 1);
        (this->*m_appendFunc)(time, track+1, ev);
    }
}

void SequenceModel::expression(int track, long time, int /*code*/, const QByteArray& text)
{
    if (!text.isEmpty()) {
        SequencerEvent *ev = new TextEvent2(text, 1);
        (this->*m_appendFunc)(time, track+1, ev);
    }
}

void SequenceModel::marker(long time, int smpte, const QByteArray &text)
{
    if (!text.isEmpty()) {
        QByteArray bam = "Time: ";
        bam.append(smpte == 0 ? "Ticks" : "SMPTE");
        bam.append("Text: ");
        bam.append(text);
        SequencerEvent *ev = new TextEvent2(bam, 6);
        (this->*m_appendFunc)(time, 0, ev);
    }
}

void SequenceModel::endOfWrk()
{
    if (m_initialTempo < 0)
        m_initialTempo = TEMPO_BPM;
}

void SequenceModel::unknownChunk(int type, const QByteArray& data)
{
    qDebug() << "unknown chunk type"
             << "dec:" << type
             << "hex:" << hex << type << dec
             << "size:" << data.length();
}

int SequenceModel::getTrackForIndex(int idx)
{
    QList<int> indexes = m_trackMap.keys();
    //qDebug() << Q_FUNC_INFO << idx << indexes;
    if (!indexes.isEmpty() && (idx < indexes.length()))
        return indexes.at(idx) + 1;
    else
        return 1;
}
