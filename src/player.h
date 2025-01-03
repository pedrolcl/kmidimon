/*
    KMidimon - ALSA sequencer based MIDI monitor
    Copyright (C) 2005-2024 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INCLUDED_PLAYER_H
#define INCLUDED_PLAYER_H

#include <drumstick/playthread.h>
#include <drumstick/alsaclient.h>
#include "sequencemodel.h"

class Player : public drumstick::ALSA::SequencerOutputThread
{
    Q_OBJECT

public:
    Player(drumstick::ALSA::MidiClient *seq, int portId);
	virtual ~Player();
    virtual bool hasNext() override;
    virtual drumstick::ALSA::SequencerEvent* nextEvent() override;
    virtual unsigned int getInitialPosition() override { return m_songPosition; }
    virtual unsigned int getEchoResolution() override { return m_echoResolution; }
    void setSong(Song* s, unsigned int division);
    void resetPosition();
    void setPosition(unsigned int pos);
    void setLastIndex(const unsigned int index) { m_lastIndex = index; }
    unsigned int getLastIndex() const { return m_lastIndex; }

protected:
    virtual void sendEchoEvent(int tick) override;
    virtual void sendSongEvent(drumstick::ALSA::SequencerEvent* ev) override;

private:
    Song *m_song;
    SongIterator* m_songIterator;
    unsigned int m_songPosition;
    unsigned int m_lastIndex;
    unsigned int m_echoResolution;
    unsigned int m_lastIndexSent;
};

#endif /*INCLUDED_PLAYER_H*/
