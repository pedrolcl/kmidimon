/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2020 Pedro Lopez-Cabanillas                        *
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

#ifndef SEQUENCEITEM_H
#define SEQUENCEITEM_H

#include <drumstick/alsaevent.h>

class SequenceItem
{
public:
    SequenceItem(double seconds,
                 unsigned int ticks,
                 unsigned int track,
                 drumstick::ALSA::SequencerEvent* ev):
    m_seconds(seconds),
    m_ticks(ticks),
    m_track(track),
    m_event(ev)
    {}

    virtual ~SequenceItem()
    {}

    bool operator==(const SequenceItem& other) const;

    double getSeconds() const { return m_seconds; }
    unsigned int  getTicks() const { return m_ticks; }
    drumstick::ALSA::SequencerEvent* getEvent() const { return m_event; }
    void deleteEvent() { delete m_event; }
    int getTrack() const { return m_track; }
    void setTrack(int track) { m_track = track; }

private:
    double m_seconds;
    unsigned int m_ticks;
    unsigned int m_track;
    drumstick::ALSA::SequencerEvent* m_event;
};

#endif /* SEQUENCEITEM_H */
