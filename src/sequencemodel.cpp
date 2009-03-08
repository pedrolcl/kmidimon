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
#include "sequencemodel.h"

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
            return i18n("Time");
        case 1:
            return i18n("Source");
        case 2:
            return i18n("Event kind");
        case 3:
            return i18n("Chan");
        case 4:
            return i18n("Data 1");
        case 5:
            return i18n("Data 2");
        }
    }
    return QVariant();
}

QModelIndex
SequenceModel::index(int row, int column,
                     const QModelIndex& /*parent*/) const
{
    if ((row < m_items.count()) && (column < 6))
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
    return 6;
}

QVariant
SequenceModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        if ( role == Qt::DisplayRole ) {
            SequenceItem item = m_items[index.row()];
            switch (index.column()) {
            case 0:
                return item.getTime();
            case 1:
                return item.getSource();
            case 2:
                return item.getKind();
            case 3:
                return item.getChannel();
            case 4:
                return item.getData1();
            case 5:
                return item.getData2();
            }
        } else
        if ( role == Qt::TextAlignmentRole ) {
            switch(index.column()) {
            case 0:
                return Qt::AlignRight;
            case 1:
                return Qt::AlignRight;
            case 2:
                return Qt::AlignLeft;
            case 3:
                return Qt::AlignRight;
            case 4:
                return Qt::AlignRight;
            case 5:
                return Qt::AlignLeft;
            }
        }
    }
    return QVariant();
}

void
SequenceModel::addItem(SequenceItem& itm)
{
    int where = m_sorted ? m_items.count() : 0;
    QModelIndex idx1 = createIndex(where, 0);
    QModelIndex idx2 = createIndex(where, 5);
    beginInsertRows(QModelIndex(), where, where);
    if (m_sorted)
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
    m_items.clear();
    endRemoveRows();
}

void
SequenceModel::saveToStream(QTextStream& str)
{
    for( int i = 0; i < m_items.count(); ++i ) {
        SequenceItem item = m_items[i];
        str << item.getTime().trimmed() << ","
            << item.getSource().trimmed() << ","
            << item.getChannel().trimmed() << ","
            << item.getKind().trimmed() << ","
            << item.getData1().trimmed() << ","
            << item.getData2().trimmed() << endl;
    }
}
