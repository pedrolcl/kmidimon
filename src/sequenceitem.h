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

#ifndef SEQUENCEITEM_H_
#define SEQUENCEITEM_H_

#include <event.h>

class SequenceItem
{
public:
    SequenceItem();
    SequenceItem(QString time,
                 QString src,
                 QString kind,
                 QString ch = QString::null,
                 QString d1 = QString::null,
                 QString d2 = QString::null):
    m_time(time),
    m_src(src),
    m_kind(kind),
    m_chan(ch),
    m_data1(d1),
    m_data2(d2) {}

    virtual ~SequenceItem();
    QString getTime() { return m_time; }
    QString getSource() { return m_src; }
    QString getKind() { return m_kind; }
    QString getChannel() { return m_chan; }
    QString getData1() { return m_data1; }
    QString getData2() { return m_data2; }
private:
    QString m_time;
    QString m_src;
    QString m_kind;
    QString m_chan;
    QString m_data1;
    QString m_data2;
};

#endif /* SEQUENCEITEM_H_ */
