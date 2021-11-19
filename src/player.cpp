/*
    KMidimon - ALSA sequencer based MIDI monitor
    Copyright (C) 2005-2021 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>

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

extern "C" {
    #include <alsa/asoundlib.h>
}

#include <drumstick/alsaqueue.h>
#include "sequenceradaptor.h"
#include "player.h"

using namespace drumstick::ALSA;

Player::Player(MidiClient *seq, int portId)
    : SequencerOutputThread(seq, portId),
    m_song(nullptr),
    m_songIterator(nullptr),
    m_songPosition(0),
    m_lastIndex(0),
    m_echoResolution(0),
    m_lastIndexSent(0)
{ }

Player::~Player()
{
    if (isRunning()) {
        stop();
    }
    if (m_songIterator != nullptr) {
        delete m_songIterator;
        m_songIterator = nullptr;
    }
}

void Player::setSong(Song* s, unsigned int division)
{
    Q_UNUSED(division)
    m_song = s;
    if (m_songIterator != nullptr) {
        delete m_songIterator;
        m_songIterator = nullptr;
    }
    if ((m_song != nullptr) && !m_song->isEmpty()) {
        m_songIterator = new SongIterator(*m_song);
        m_echoResolution = 0;
        resetPosition();
    }
}

void Player::resetPosition()
{
    if ((m_song != nullptr) && (m_songIterator != nullptr)) {
        m_songIterator->toFront();
        m_songPosition = 0;
        m_lastIndex = 0;
        m_lastIndexSent = 0;
    }
}

void Player::setPosition(unsigned int pos)
{
    if (m_songIterator != nullptr) {
        m_songPosition = pos;
        m_songIterator->toFront();
        while (m_songIterator->hasNext() &&
              (m_songIterator->next().getEvent()->getTick() < pos));
        if (m_songIterator->hasPrevious())
            m_songIterator->previous();
    }
}

bool Player::hasNext()
{
    if (m_songIterator == nullptr)
        return false;
    bool res = m_songIterator->hasNext();
    bool muted = false;
    if (res) do {
        SequenceItem e = m_songIterator->peekNext();
        muted = m_song->mutedState(e.getTrack());
        if (muted) {
            e = m_songIterator->next();
            res = m_songIterator->hasNext();
        }
    } while (res && muted);
    return res;
}

SequencerEvent* Player::nextEvent()
{
    m_lastIndex = 0;
    if (m_songIterator == nullptr) {
        return nullptr;
    }
    SequenceItem itm = m_songIterator->next();
    bool muted = m_song->mutedState(itm.getTrack());
    while (muted && m_songIterator->hasNext()) {
        itm = m_songIterator->next();
        muted = m_song->mutedState(itm.getTrack());
    }
    if (muted) {
        return nullptr;
    }
    m_lastIndex = m_song->indexOf(itm);
    return itm.getEvent();
}

void
Player::sendEchoEvent(int tick)
{
    Q_UNUSED(tick)
}

void Player::sendSongEvent(drumstick::ALSA::SequencerEvent *ev)
{
    SequencerOutputThread::sendSongEvent(ev);
    if (!stopRequested() && (m_lastIndex > m_lastIndexSent)) {
        emit signalTicks(m_lastIndex);
        m_lastIndexSent = m_lastIndex;
    }
}
