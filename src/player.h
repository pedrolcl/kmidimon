/*
    KMidimon - ALSA sequencer based MIDI monitor
    Copyright (C) 2005-2010 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef INCLUDED_PLAYER_H
#define INCLUDED_PLAYER_H

#include "playthread.h"
#include "alsaclient.h"
#include "sequencemodel.h"

using namespace drumstick;

class Player : public SequencerOutputThread
{
    Q_OBJECT

public:
	Player(MidiClient *seq, int portId);
	virtual ~Player();
    virtual void run();
    virtual bool hasNext();
    virtual SequencerEvent* nextEvent();
    virtual unsigned int getInitialPosition() { return m_songPosition; }
    virtual unsigned int getEchoResolution() { return m_echoResolution; }
    void setSong(Song* s, unsigned int division);
    void resetPosition();
    void setPosition(unsigned int pos);
    void setLastIndex(const unsigned int index) { m_lastIndex = index; }
    unsigned int getLastIndex() const { return m_lastIndex; }
    void setLoop(bool enabled);

protected:
    virtual void sendEchoEvent(int tick);

private:
    Song* m_song;
    SongIterator* m_songIterator;
    unsigned int m_songPosition;
    unsigned int m_lastIndex;
    unsigned int m_echoResolution;
    bool m_loop;
};

#endif /*INCLUDED_PLAYER_H*/
