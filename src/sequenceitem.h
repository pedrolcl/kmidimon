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

using namespace ALSA::Sequencer;

class SequenceItem
{
public:
    SequenceItem(double seconds,
                 unsigned int ticks,
                 SequencerEvent* ev):
    m_seconds(seconds),
    m_ticks(ticks),
    m_event(ev)
    {}

    virtual ~SequenceItem()
    {}

    double getSeconds() const { return m_seconds; }
    unsigned int  getTicks() const { return m_ticks; }
    const SequencerEvent* getEvent() const { return m_event; }
    void deleteEvent() { delete m_event; }
    int getTag() const;
    void setTag(int tag);

private:
    double m_seconds;
    unsigned int m_ticks;
    SequencerEvent* m_event;
};

#endif /* SEQUENCEITEM_H_ */
