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

#include <klocale.h>
#include <event.h>
#include "sequencemodel.h"
#include "kmidimon.h"

SequenceModel::~SequenceModel()
{
    clear();
}

Qt::ItemFlags
SequenceModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;
    return Qt::ItemIsEnabled;
    //return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
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
            return i18n("Source");
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
    int where = m_ordered ? m_items.count() : 0;
    QModelIndex idx1 = createIndex(where, 0);
    QModelIndex idx2 = createIndex(where, 5);
    beginInsertRows(QModelIndex(), where, where);
    if (m_ordered)
        m_items.append(itm);
    else
        m_items.insert(0, itm);
    //emit dataChanged(idx1, idx2);
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
}

void
SequenceModel::saveToStream(QTextStream& str)
{
    for( int i = 0; i < m_items.count(); ++i ) {
        SequenceItem itm = m_items[i];
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
    const SysExEvent *sev = dynamic_cast<const SysExEvent*>(ev);
    if (sev != NULL) {
        if (m_translateSysex) {
            unsigned char *ptr = (unsigned char *) sev->getData();
            if (sev->getLength() < 6) return QString::null;
            if (*ptr++ != 0xf0) return QString::null;
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
    return QString::null;
}

QString
SequenceModel::sysex_chan(const SequencerEvent *ev) const
{
    const SysExEvent *sev = dynamic_cast<const SysExEvent*>(ev);
    if (sev != NULL) {
        if (m_translateSysex) {
            unsigned char *ptr = (unsigned char *) sev->getData();
            if (sev->getLength() < 6) return QString::null;
            if (*ptr++ != 0xf0) return QString::null;
            *ptr++;
            int deviceId = *ptr++;
            if ( deviceId == 0x7f )
                return i18n("broadcast");
            else
                return i18n("device %1").arg(deviceId);
        }
        return "-";
    }
    return QString::null;
}

QString
SequenceModel::sysex_data1(const SequencerEvent *ev) const
{
    const SysExEvent *sev = dynamic_cast<const SysExEvent*>(ev);
    if (sev != NULL) {
        if (m_translateSysex) {
            unsigned char *ptr = (unsigned char *) sev->getData();
            if (sev->getLength() < 6) return QString::null;
            if (*ptr++ != 0xf0) return QString::null;
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
    return QString::null;
}

QString
SequenceModel::sysex_mtc_setup(const int id) const
{
    switch (id) {
        case 0x00:
            return i18n("Special");
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
    return QString::null;
}

QString
SequenceModel::sysex_mtc(int id, int length, unsigned char *ptr) const
{
    switch (id) {
        case 0x01:
            if (length >= 10)
                return i18n("Full Frame: %1 %2 %3 %4")
                        .arg(ptr[0]).arg(ptr[1]).arg(ptr[2]).arg(ptr[3]);
            break;
        case 0x02:
            if (length >= 15)
                return i18n("User Bits: %1 %2 %3 %4 %5 %6 %7 %8 %9")
                        .arg(ptr[0]).arg(ptr[1]).arg(ptr[2]).arg(ptr[3])
                        .arg(ptr[4]).arg(ptr[5]).arg(ptr[6]).arg(ptr[7])
                        .arg(ptr[8]);
            break;
        default:
            break;
    }
    return QString::null;
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
            return i18n("Locate: %1 %2 %3 %4 %5")
                    .arg(ptr[0]).arg(ptr[1]).arg(ptr[2]).arg(ptr[3])
                    .arg(ptr[4]);
        }
        break;
    default:
        break;
    }
    return QString::null;
}

QString
SequenceModel::sysex_data2(const SequencerEvent *ev) const
{
    const SysExEvent *sev = dynamic_cast<const SysExEvent*>(ev);
    if (sev != NULL) {
        if (m_translateSysex) {
            int val;
            unsigned char *ptr = (unsigned char *) sev->getData();
            if (sev->getLength() < 6) return QString::null;
            if (*ptr++ != 0xf0) return QString::null;
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
                                return i18n("Identity Reply: %1 %2 %3 %4 %5 %6 %7 %8 %9")
                                        .arg(ptr[0]).arg(ptr[1]).arg(ptr[2]).arg(ptr[3])
                                        .arg(ptr[4]).arg(ptr[5]).arg(ptr[6]).arg(ptr[7])
                                        .arg(ptr[8]);
                                break;
                            default:
                                break;
                        }
                        break;
                    case 0x08:
                        if (sev->getLength() >= 7)
                        switch (subId2) {
                            case 0x00:
                                return i18n("Dump Request: %1").arg(*ptr++);
                            case 0x01:
                                return i18n("Bulk Dump: %1").arg(*ptr++);
                            case 0x02:
                                return i18n("Note Change: %1").arg(*ptr++);
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
                                    return i18n("Bar Marker: %1").arg(val - 8192);
                                }
                                break;
                            case 0x02:
                            case 0x42:
                                if (sev->getLength() >= 9)
                                return i18n("Time Signature: %1 (%2/%3)")
                                        .arg(ptr[0]).arg(ptr[1]).arg(ptr[2]);
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
                                    return i18n("Volume: %1").arg(val);
                                case 0x02:
                                    return i18n("Balance: %1").arg(val - 8192);
                                default:
                                    break;
                            }
                        }
                        break;
                    case 0x06:
                        return sysex_mmc(subId2, sev->getLength(), ptr);
                    default:
                        return QString::null;
                }
            }
        }
        unsigned int i;
        unsigned char *data = (unsigned char *) sev->getData();
        QString text;
        for (i = 0; i < sev->getLength(); ++i) {
            QString h(QString("%1").arg(data[i], 0, 16));
            if (h.length() < 2) h.prepend('0');
            h.prepend(' ');
            text.append(h);
        }
        return text.trimmed();
    }
    return QString::null;
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
        if (name != QString::null)
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
    const PortEvent* pe = dynamic_cast<const PortEvent*>(ev);
    if (pe != NULL)
        return QString("%1:%2").arg(client_name(pe->getClient()))
                               .arg(pe->getPort());
    else
        return QString::null;
}

QString
SequenceModel::event_client(const SequencerEvent *ev) const
{
    const ClientEvent* ce = dynamic_cast<const ClientEvent*>(ev);
    if (ce != NULL)
        return client_name(ce->getClient());
    else
        return QString::null;
}

QString
SequenceModel::event_sender(const SequencerEvent *ev) const
{
    const SubscriptionEvent* se = dynamic_cast<const SubscriptionEvent*>(ev);
    if (se != NULL)
        return QString("%1:%2").arg(client_name(se->getSenderClient()))
                               .arg(se->getSenderPort());
    else
        return QString::null;
}

QString
SequenceModel::event_dest(const SequencerEvent *ev) const
{
    const SubscriptionEvent* se = dynamic_cast<const SubscriptionEvent*>(ev);
    if (se != NULL)
        return QString("%1:%2").arg(client_name(se->getDestClient()))
                               .arg(se->getDestPort());
    else
        return QString::null;
}

QString
SequenceModel::common_param(const SequencerEvent *ev) const
{
    const ValueEvent* ve = dynamic_cast<const ValueEvent*>(ev);
    if (ve != NULL)
        return QString("%1").arg(ve->getValue());
    else
        return QString::null;
}

QString
SequenceModel::event_kind(const SequencerEvent *ev) const
{
    switch (ev->getSequencerType()) {
    /* MIDI Channel events */
    case SND_SEQ_EVENT_NOTEON:
        return i18n("Note on");
    case SND_SEQ_EVENT_NOTEOFF:
        return i18n("Note off");
    case SND_SEQ_EVENT_KEYPRESS:
        return i18n("Polyphonic aftertouch");
    case SND_SEQ_EVENT_CONTROLLER:
        return i18n("Control change");
    case SND_SEQ_EVENT_PGMCHANGE:
        return i18n("Program change");
    case SND_SEQ_EVENT_CHANPRESS:
        return i18n("Channel aftertouch");
    case SND_SEQ_EVENT_PITCHBEND:
        return i18n("Pitch bend");
    case SND_SEQ_EVENT_CONTROL14:
        return i18n("Control change");
    case SND_SEQ_EVENT_NONREGPARAM:
        return i18n("Non-registered parameter");
    case SND_SEQ_EVENT_REGPARAM:
        return i18n("Registered parameter");
        /* MIDI Common events */
    case SND_SEQ_EVENT_SYSEX:
        return sysex_type(ev);
    case SND_SEQ_EVENT_SONGPOS:
        return i18n("Song Position");
    case SND_SEQ_EVENT_SONGSEL:
        return i18n("Song Selection");
    case SND_SEQ_EVENT_QFRAME:
        return i18n("MTC Quarter Frame");
    case SND_SEQ_EVENT_TUNE_REQUEST:
        return i18n("Tune Request");
        /* MIDI Realtime Events */
    case SND_SEQ_EVENT_START:
        return i18n("Start");
    case SND_SEQ_EVENT_CONTINUE:
        return i18n("Continue");
    case SND_SEQ_EVENT_STOP:
        return i18n("Stop");
    case SND_SEQ_EVENT_CLOCK:
        return i18n("Clock");
    case SND_SEQ_EVENT_TICK:
        return i18n("Tick");
    case SND_SEQ_EVENT_RESET:
        return i18n("Reset");
    case SND_SEQ_EVENT_SENSING:
        return i18n("Active Sensing");
        /* ALSA Client/Port events */
    case SND_SEQ_EVENT_PORT_START:
        return i18n("ALSA Port start");
    case SND_SEQ_EVENT_PORT_EXIT:
        return i18n("ALSA Port exit");
    case SND_SEQ_EVENT_PORT_CHANGE:
        return i18n("ALSA Port change");
    case SND_SEQ_EVENT_CLIENT_START:
        return i18n("ALSA Client start");
    case SND_SEQ_EVENT_CLIENT_EXIT:
        return i18n("ALSA Client exit");
    case SND_SEQ_EVENT_CLIENT_CHANGE:
        return i18n("ALSA Client change");
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
        return i18n("ALSA Port subscribed");
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
        return i18n("ALSA Port unsubscribed");
        /* Other events */
    default:
         return QString("Event type %1").arg(ev->getSequencerType());
    }
    return QString::null;
}

QString
SequenceModel::event_channel(const SequencerEvent *ev) const
{
    const ChannelEvent* che = dynamic_cast<const ChannelEvent*>(ev);
    if (che != NULL)
        return QString("%1").arg(che->getChannel()+1);
    else
        return QString::null;
}

QString
SequenceModel::note_key(const SequencerEvent* ev) const
{
    const KeyEvent* ke = dynamic_cast<const KeyEvent*>(ev);
    if (ke != NULL)
        return QString("%1").arg(ke->getKey());
    else
        return QString::null;
}

QString
SequenceModel::program_number(const SequencerEvent* ev) const
{
    const ProgramChangeEvent* pc = dynamic_cast<const ProgramChangeEvent*>(ev);
    if (pc != NULL)
        return QString("%1").arg(pc->getValue());
    else
        return QString::null;
}

QString
SequenceModel::note_velocity(const SequencerEvent* ev) const
{
    const KeyEvent* ke = dynamic_cast<const KeyEvent*>(ev);
    if (ke != NULL)
        return QString("%1").arg(ke->getVelocity());
    else
        return QString::null;
}

QString
SequenceModel::control_param(const SequencerEvent* ev) const
{
    const ControllerEvent* ce = dynamic_cast<const ControllerEvent*>(ev);
    if (ce != NULL)
        return QString("%1").arg(ce->getParam());
    else
        return QString::null;
}

QString
SequenceModel::control_value(const SequencerEvent* ev) const
{
    const ControllerEvent* ce = dynamic_cast<const ControllerEvent*>(ev);
    if (ce != NULL)
        return QString("%1").arg(ce->getValue());
    else
        return QString::null;
}

QString
SequenceModel::pitchbend_value(const SequencerEvent* ev) const
{
    const PitchBendEvent* pe = dynamic_cast<const PitchBendEvent*>(ev);
    if (pe != NULL)
        return QString("%1").arg(pe->getValue());
    else
        return QString::null;
}

QString
SequenceModel::chanpress_value(const SequencerEvent* ev) const
{
    const ChanPressEvent* cp = dynamic_cast<const ChanPressEvent*>(ev);
    if (cp != NULL)
        return QString("%1").arg(cp->getValue());
    else
        return QString::null;
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

       /* Other events */
    default:
        return QString::null;
    }
    return QString::null;
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

       /* Other events */
    default:
        return QString::null;
    }
    return QString::null;
}

const SequencerEvent*
SequenceModel::getEvent(const int row) const
{
    if ((row >= 0) && (row < m_items.count()))
        return m_items[row].getEvent();
    return NULL;
}
