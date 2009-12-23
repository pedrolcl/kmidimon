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

#include "sequencemodel.h"
#include "sequenceradaptor.h"
#include "kmidimon.h"
#include "eventfilter.h"
#include <cmath>
#include <QFile>
#include <QDataStream>
#include <QListIterator>
#include <QFileInfo>
#include <QTime>
#include <QTextCodec>
#include <klocale.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <kcharsets.h>

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
        m_currentTrack(0),
        m_currentRow(0),
        m_portId(0),
        m_queueId(0),
        m_format(0),
        m_ntrks(1),
        m_division(RESOLUTION),
        m_initialTempo(TEMPO_BPM),
        m_duration(0),
        m_ins(NULL),
        m_ins2(NULL),
        m_filter(NULL)
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
                   SLOT(ctlChangeEvent(int,int,int)));
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
                   SLOT(keySigEvent(int,int)));
    connect(m_smf, SIGNAL(signalSMFError(const QString&)),
                   SLOT(errorHandler(const QString&)));
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

    for(int i=0; i<16; ++i) {
        m_lastBank[i] = 0;
        m_lastPatch[i] = 0;
    }

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
SequenceModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_items.count();
}

int
SequenceModel::columnCount(const QModelIndex& /*parent*/) const
{
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
    for ( it = m_items.begin(); it != m_items.end(); ++it ) {
        SequenceItem itm = *it;
        itm.deleteEvent();
    }
    m_items.clear();
    endRemoveRows();
    m_format = 0;
    m_ntrks = 1;
    m_division = RESOLUTION;
    m_initialTempo = TEMPO_BPM;
    m_currentTrack = 0;
    m_currentRow = 0;
    m_duration = 0;
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
                << event_data2(ev).trimmed() << endl;
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
            if (*ptr++ != 0xf0) return QString();
            int msgId = *ptr++;
            switch (msgId) {
            case 0x7e:
                return i18n("Universal Non Real Time SysEx");
            case 0x7f:
                return i18n("Universal Real Time SysEx");
            default:
                break;
            }
        }
        return i18n("System exclusive");
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
            if (*ptr++ != 0xf0) return QString();
            *ptr++;
            unsigned char deviceId = *ptr++;
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
            if (*ptr++ != 0xf0) return QString();
            int msgId = *ptr++;
            *ptr++;
            int subId1 = *ptr++;
            if (msgId == 0x7e) { // Universal Non Real Time
                switch (subId1) {
                    case 0x01:
                    case 0x02:
                    case 0x03:
                        return i18n("Sample Dump");
                    case 0x04:
                        return i18n("MTC Setup");
                    case 0x05:
                        return i18n("Sample Dump");
                    case 0x06:
                        return i18n("Gen.Info");
                    case 0x08:
                        return i18n("Tuning");
                    case 0x09:
                        return i18n("GM Mode");
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
                    case 0x03:
                        return i18n("Notation");
                    case 0x04:
                        return i18n("GM Master");
                    case 0x06:
                        return i18n("MMC");
                    default:
                        break;
                }
            }
        }
        return QString("%1").arg(sev->getLength());
    }
    return QString();
}

QString
SequenceModel::sysex_mtc_setup(const int id) const
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
            return i18n("Delete Cue Points");
        case 0x0e:
            return i18n("Event Name");
        default:
            break;
    }
    return QString();
}

QString
SequenceModel::sysex_mtc(int id, int length, unsigned char *ptr) const
{
    switch (id) {
        case 0x01:
            if (length >= 10)
                return i18n("Full Frame: %1 %2 %3 %4",
                        ptr[0], ptr[1], ptr[2], ptr[3]);
            break;
        case 0x02:
            if (length >= 15)
                return i18n("User Bits: %1 %2 %3 %4 %5 %6 %7 %8 %9",
                        ptr[0], ptr[1], ptr[2], ptr[3],
                        ptr[4], ptr[5], ptr[6], ptr[7],
                        ptr[8]);
            break;
        default:
            break;
    }
    return QString();
}

QString
SequenceModel::sysex_mmc(int id, int length, unsigned char *ptr) const
{
    int i, len, cmd;
    QString data;
    switch (id) {
    case 0x01:
        return i18n("Stop");
    case 0x02:
        return i18n("Play");
    case 0x03:
        return i18n("Deferred Play");
    case 0x04:
        return i18n("Fast Forward");
    case 0x05:
        return i18n("Rewind");
    case 0x06:
        return i18n("Punch In");
    case 0x07:
        return i18n("Punch out");
    case 0x09:
        return i18n("Pause");
    case 0x0a:
        return i18n("Eject");
    case 0x40:
        len = *ptr++;
        if (length >= (7+len)) {
            cmd = *ptr++;
            switch (cmd) {
            case 0x4f:
                data = i18n("Track Record Ready:");
                break;
            case 0x52:
                data = i18n("Track Sync Monitor:");
                break;
            case 0x53:
                data = i18n("Track Input Monitor:");
                break;
            case 0x62:
                data = i18n("Track Mute:");
                break;
            default:
                break;
            }
            for (i=1; i < len; ++i) {
                data.append(QString(" %1").arg(*ptr++, 0, 16));
            }
            return data;
        }
        break;
    case 0x44:
        if (length >= 13) {
            *ptr++; *ptr++;
            return i18n("Locate: %1 %2 %3 %4 %5",
                    ptr[0], ptr[1], ptr[2], ptr[3],
                    ptr[4] );
        }
        break;
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
            int val;
            unsigned char *ptr = (unsigned char *) sev->getData();
            if (sev->getLength() < 6) return QString();
            if (*ptr++ != 0xf0) return QString();
            int msgId = *ptr++;
            *ptr++;
            int subId1 = *ptr++;
            int subId2 = *ptr++;
            if (msgId == 0x7e) { // Universal Non Real Time
                switch (subId1) {
                    case 0x01:
                        return i18n("Header");
                    case 0x02:
                        return i18n("Data Packet");
                    case 0x03:
                        return i18n("Request");
                    case 0x04:
                        return sysex_mtc_setup(subId2);
                        break;
                    case 0x05:
                        switch (subId2) {
                            case 0x01:
                                return i18n("Multiple Loop Points");
                            case 0x02:
                                return i18n("Loop Points Request");
                            default:
                                break;
                        }
                        break;
                    case 0x06:
                        switch (subId2) {
                            case 0x01:
                                return i18n("Identity Request");
                            case 0x02:
                                if (sev->getLength() >= 15)
                                return i18n("Identity Reply: %1 %2 %3 %4 %5 %6 %7 %8 %9",
                                        ptr[0], ptr[1], ptr[2], ptr[3],
                                        ptr[4], ptr[5], ptr[6], ptr[7],
                                        ptr[8]);
                                break;
                            default:
                                break;
                        }
                        break;
                    case 0x08:
                        if (sev->getLength() >= 7)
                        switch (subId2) {
                            case 0x00:
                                return i18n("Dump Request: %1", *ptr++);
                            case 0x01:
                                return i18n("Bulk Dump: %1", *ptr++);
                            case 0x02:
                                return i18n("Note Change: %1", *ptr++);
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
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
            } else
            if (msgId == 0x7f) { // Universal Real Time
                switch (subId1) {
                    case 0x01:
                        return sysex_mtc(subId2, sev->getLength(), ptr);
                    case 0x03:
                        switch (subId2) {
                            case 0x01:
                                if (sev->getLength() >= 8) {
                                    val = *ptr++;
                                    val += (*ptr++ * 128);
                                    return i18n("Bar Marker: %1", val - 8192);
                                }
                                break;
                            case 0x02:
                            case 0x42:
                                if (sev->getLength() >= 9)
                                return i18n("Time Signature: %1 (%2/%3)",
                                        ptr[0], ptr[1], ptr[2]);
                                break;
                            default:
                                break;
                        }
                        break;
                    case 0x04:
                        if (sev->getLength() >= 8) {
                            val = *ptr++;
                            val += (*ptr++ * 128);
                            switch (subId2) {
                                case 0x01:
                                    return i18nc("sound volume","Volume: %1", val);
                                case 0x02:
                                    return i18nc("sound balance","Balance: %1", val - 8192);
                                default:
                                    break;
                            }
                        }
                        break;
                    case 0x06:
                        return sysex_mmc(subId2, sev->getLength(), ptr);
                    default:
                        return QString();
                }
            }
        }
        unsigned int i;
        unsigned char *data = (unsigned char *) sev->getData();
        QString text;
        for (i = 0; i < sev->getLength(); ++i) {
            QString h = QString::number(data[i], 16);
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
    return QString("%1").arg(ev->getTick());
}

QString
SequenceModel::event_time(const SequenceItem& itm) const
{
    //return QString("%1").arg(itm.getTicks());
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
    return QString("%1").arg(client_number);
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
        return QString("%1").arg(ve->getValue());
    else
        return QString();
}

QString
SequenceModel::event_kind(const SequencerEvent *ev) const
{
    QString res;
    if (m_filter != NULL)
        res = m_filter->getName(ev->getSequencerType());
    if (res.isEmpty())
        res = QString("Event type %1").arg(ev->getSequencerType());
    return res;
}

QString
SequenceModel::event_channel(const SequencerEvent *ev) const
{
    if (SequencerEvent::isChannel(ev)) {
        const ChannelEvent* che = static_cast<const ChannelEvent*>(ev);
        if (che != NULL)
            return QString("%1").arg(che->getChannel()+1);
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
            return QString("%1").arg(ke->getKey());
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
        return QString("%1").arg(pc->getValue());
    }
    return QString();
}

QString
SequenceModel::note_velocity(const SequencerEvent* ev) const
{
    const KeyEvent* ke = static_cast<const KeyEvent*>(ev);
    if (ke != NULL)
        return QString("%1").arg(ke->getVelocity());
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
        return QString("%1").arg(ce->getParam());
    }
    return QString();
}

QString
SequenceModel::control_value(const SequencerEvent* ev) const
{
    const ControllerEvent* ce = static_cast<const ControllerEvent*>(ev);
    if (ce != NULL)
        return QString("%1").arg(ce->getValue());
    else
        return QString();
}

QString
SequenceModel::pitchbend_value(const SequencerEvent* ev) const
{
    const PitchBendEvent* pe = static_cast<const PitchBendEvent*>(ev);
    if (pe != NULL)
        return QString("%1").arg(pe->getValue());
    else
        return QString();
}

QString
SequenceModel::chanpress_value(const SequencerEvent* ev) const
{
    const ChanPressEvent* cp = static_cast<const ChanPressEvent*>(ev);
    if (cp != NULL)
        return QString("%1").arg(cp->getValue());
    else
        return QString();
}

QString
SequenceModel::tempo_bpm(const SequencerEvent *ev) const
{
    const TempoEvent* te = static_cast<const TempoEvent*>(ev);
    if (te != NULL)
        return QString("%1 bpm").arg(6e7 / te->getValue(), 0, 'f', 1);
    else
        return QString();
}

QString
SequenceModel::tempo_npt(const SequencerEvent *ev) const
{
    const TempoEvent* te = static_cast<const TempoEvent*>(ev);
    if (te != NULL)
        return QString("%1").arg(te->getValue());
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
            return QString("%1").arg( te->getTextType() );
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
SequenceModel::time_sig(const SequencerEvent *ev) const
{
    return i18n("%1/%2, %3 clocks per click, %4 32nd per quarter",
            ev->getRaw8(0),
            pow(2, ev->getRaw8(1)),
            ev->getRaw8(2),
            ev->getRaw8(3) );
}

QString
SequenceModel::key_sig(const SequencerEvent *ev) const
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
    QString tone;
    if (abs(s) < 8)
        tone = ( ev->getRaw8(1) == 0 ? tmaj[s + 7] : tmin[s + 7] );
    return i18n("%1%2, %3 %4", abs(s),
            s < 0 ? QChar(0x266D) : QChar(0x266F), //s < 0 ? "♭" : "♯"
            tone,
            ev->getRaw8(1) == 0 ? i18nc("major mode scale","major") : i18nc("minor mode scale","minor"));
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
        return QString("%1").arg(ev->getRaw8(0));

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
    case SND_SEQ_EVENT_NOTEON:
    case SND_SEQ_EVENT_NOTEOFF:
    case SND_SEQ_EVENT_KEYPRESS:
        return note_velocity(ev);

    case SND_SEQ_EVENT_CONTROLLER:
    case SND_SEQ_EVENT_CONTROL14:
    case SND_SEQ_EVENT_NONREGPARAM:
    case SND_SEQ_EVENT_REGPARAM:
        return control_value(ev);

    case SND_SEQ_EVENT_SYSEX:
        return sysex_data2(ev);

    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
       return event_dest(ev);

    case SND_SEQ_EVENT_TEMPO:
        return tempo_bpm(ev);

    case SND_SEQ_EVENT_USR_VAR0:
        return text_data(ev);

    case SND_SEQ_EVENT_TIMESIGN:
        return time_sig(ev);

    case SND_SEQ_EVENT_KEYSIGN:
        return key_sig(ev);

    case SND_SEQ_EVENT_USR4:
        return smpte(ev);

    case SND_SEQ_EVENT_USR_VAR1:
    case SND_SEQ_EVENT_USR_VAR2:
        return var_event(ev);

        /* Other events */
    default:
        return QString();
    }
    return QString();
}

const SequenceItem*
SequenceModel::getItem(const int row) const
{
    if (!m_items.empty() && (row >= 0) && (row < m_items.size()))
        return &m_items[row];
    return NULL;
}

const SequencerEvent*
SequenceModel::getEvent(const int row) const
{
    if (!m_items.empty() && (row >= 0) && (row < m_items.size()))
        return m_items[row].getEvent();
    return NULL;
}

void
SequenceModel::loadFromFile(const QString& path)
{
    clear();
    m_loadedSong.clear();
    m_currentTrack = -1;
    m_initialTempo = -1;
    m_smf->readFromFile(path);
    m_loadedSong.sort();
    beginInsertRows(QModelIndex(), 0, m_loadedSong.count() - 1);
    m_items += m_loadedSong;
    endInsertRows();
    m_items.setLast(m_loadedSong.getLast());
    m_loadedSong.clear();
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
        m_smf->setDivision(m_division);
        m_smf->setFileFormat(1);
        m_smf->setTracks(m_ntrks);
        m_smf->writeToFile(path);
    }
}

void
SequenceModel::appendEvent(SequencerEvent* ev)
{
    long ticks = m_smf->getCurrentTime();
    double seconds = m_smf->getRealTime() / 1600.0;
    ev->setSource(m_portId);
    ev->scheduleTick(m_queueId, ticks, false);
    ev->setTag(m_currentTrack);
    if (ev->getSequencerType() != SND_SEQ_EVENT_TEMPO) {
        ev->setSubscribers();
    }
    SequenceItem itm(seconds, ticks, m_currentTrack, ev);
    m_loadedSong.append(itm);
    m_loadedSong.setLast(ticks);
    emit loadProgress(m_smf->getFilePos());
    KApplication::processEvents();
}

void
SequenceModel::headerEvent(int format, int ntrks, int division)
{
    m_format = format;
    m_ntrks = ntrks;
    m_division = division;
}

void
SequenceModel::trackStartEvent()
{
    m_currentTrack++;
    m_loadedSong.setMutedState(m_currentTrack, false);
    emit loadProgress(m_smf->getFilePos());
}

void
SequenceModel::trackEndEvent()
{
    long ticks = m_smf->getCurrentTime();
    m_loadedSong.setLast(ticks);
    emit loadProgress(m_smf->getFilePos());
}

void
SequenceModel::endOfTrackEvent()
{
    double seconds = m_smf->getRealTime() / 1600.0;
    if (seconds > m_duration)
        m_duration = seconds;
    emit loadProgress(m_smf->getFilePos());
}

void
SequenceModel::noteOnEvent(int chan, int pitch, int vol)
{
    SequencerEvent* ev = new NoteOnEvent(chan, pitch, vol);
    appendEvent(ev);
}

void
SequenceModel::noteOffEvent(int chan, int pitch, int vol)
{
    SequencerEvent* ev = new NoteOffEvent(chan, pitch, vol);
    appendEvent(ev);
}

void
SequenceModel::keyPressEvent(int chan, int pitch, int press)
{
    SequencerEvent* ev = new KeyPressEvent(chan, pitch, press);
    appendEvent(ev);
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

        if (ctl == MIDI_CTL_MSB_BANK)
            m_lastCtlMSB = value;
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

    SequencerEvent* ev = new ControllerEvent(chan, ctl, value);
    appendEvent(ev);
}

void
SequenceModel::pitchBendEvent(int chan, int value)
{
    SequencerEvent* ev = new PitchBendEvent(chan, value);
    appendEvent(ev);
}

void
SequenceModel::programEvent(int chan, int patch)
{
    m_lastPatch[chan] = patch;
    SequencerEvent* ev = new ProgramChangeEvent(chan, patch);
    appendEvent(ev);
}

void
SequenceModel::chanPressEvent(int chan, int press)
{
    SequencerEvent* ev = new ChanPressEvent(chan, press);
    appendEvent(ev);
}

void
SequenceModel::sysexEvent(const QByteArray& data)
{
    SequencerEvent* ev = new SysExEvent(data);
    appendEvent(ev);
}

void
SequenceModel::seqSpecificEvent(const QByteArray& data)
{
    SequencerEvent* ev = new VariableEvent(data);
    ev->setSequencerType(SND_SEQ_EVENT_USR_VAR1);
    appendEvent(ev);
}

void
SequenceModel::metaMiscEvent(int typ, const QByteArray& data)
{
    QByteArray dataCopy;
    dataCopy.append(typ);
    dataCopy.append(data);
    SequencerEvent* ev = new VariableEvent(dataCopy);
    ev->setSequencerType(SND_SEQ_EVENT_USR_VAR2);
    appendEvent(ev);
}

void
SequenceModel::seqNum(int seq)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_USR1);
    ev->setRaw8(0, seq);
    appendEvent(ev);
}

void
SequenceModel::forcedChannel(int channel)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_USR2);
    ev->setRaw8(0, channel);
    appendEvent(ev);
}

void
SequenceModel::forcedPort(int port)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_USR3);
    ev->setRaw8(0, port);
    appendEvent(ev);
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
    appendEvent(ev);
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
    appendEvent(ev);
}

void
SequenceModel::keySigEvent(int b0, int b1)
{
    SequencerEvent* ev = new SequencerEvent();
    ev->setSequencerType(SND_SEQ_EVENT_KEYSIGN);
    ev->setRaw8(0, b0);
    ev->setRaw8(1, b1);
    appendEvent(ev);
    m_useFlats = (b0 < 0);
}

void
SequenceModel::textEvent(int type, const QString& data)
{
    SequencerEvent* ev = new TextEvent(data, type);
    appendEvent(ev);
}

void
SequenceModel::tempoEvent(int tempo)
{
    if ( m_initialTempo < 0 )
    {
        m_initialTempo = round( 6e7 / tempo );
    }
    SequencerEvent* ev = new TempoEvent(m_queueId, tempo);
    appendEvent(ev);
}

void
SequenceModel::errorHandler(const QString& errorStr)
{
    qWarning() << "*** Warning! " << errorStr
               << " at file offset " << m_smf->getFilePos();
}

void
SequenceModel::trackHandler(int track)
{
    unsigned int delta, last_tick = 0;
    foreach ( const SequenceItem& itm, m_items ) {
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
    delta = m_items.getLast() - last_tick;
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
        m_encoding = encoding;
    }
}
