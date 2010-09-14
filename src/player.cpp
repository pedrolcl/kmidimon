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

#include "player.h"
#include <alsaqueue.h>
#include <kdebug.h>

Player::Player(MidiClient *seq, int portId)
    : SequencerOutputThread(seq, portId),
    m_song(0),
    m_songIterator(0),
    m_songPosition(0),
    m_lastIndex(0),
    m_echoResolution(0),
    m_loop(false)
{ }

Player::~Player()
{
    if (isRunning()) {
        stop();
    }
    if (m_songIterator != NULL) {
        delete m_songIterator;
    }
}

void Player::setSong(Song* s, unsigned int /*division*/)
{
    m_song = s;
    if (m_songIterator != NULL) {
        delete m_songIterator;
    }
    if (m_song != NULL) {
        m_songIterator = new SongIterator(*m_song);
        //m_echoResolution = division / 24;
        resetPosition();
    }
}

void Player::resetPosition()
{
    if ((m_song != NULL) && (m_songIterator != NULL)) {
        m_songIterator->toFront();
        m_songPosition = 0;
        m_lastIndex = 0;
    }
}

void Player::setPosition(unsigned int pos)
{
    if (m_songIterator != NULL) {
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
    if (m_songIterator == NULL)
        return false;
    return m_songIterator->hasNext();
}

SequencerEvent* Player::nextEvent()
{
    if (m_songIterator == NULL)
        return NULL;
    SequenceItem itm = m_songIterator->next();
    m_lastIndex = m_song->indexOf(itm);
    return itm.getEvent();
}

void
Player::sendEchoEvent(int tick)
{
    if (!stopRequested() && m_MidiClient != NULL) {
        SystemEvent ev(SND_SEQ_EVENT_USR0);
        ev.setRaw32(0, m_lastIndex);
        ev.setSource(m_PortId);
        ev.setDestination(m_MidiClient->getClientId(), m_PortId);
        ev.scheduleTick(m_QueueId, tick, false);
        sendSongEvent(&ev);
    }
}

void Player::run()
{
    unsigned int last_tick, final_tick = m_song->getLast();
    if (m_MidiClient != NULL) {
        try  {
            m_npfds = snd_seq_poll_descriptors_count(m_MidiClient->getHandle(), POLLOUT);
            m_pfds = (pollfd*) alloca(m_npfds * sizeof(pollfd));
            snd_seq_poll_descriptors(m_MidiClient->getHandle(), m_pfds, m_npfds, POLLOUT);
            last_tick = getInitialPosition();
            if (last_tick == 0) {
                m_Queue->start();
            } else {
                m_Queue->setTickPosition(last_tick);
                m_Queue->continueRunning();
            }
            while (!stopRequested() && hasNext()) {
                SequencerEvent* ev = nextEvent();
                /*if (getEchoResolution() > 0) {
                    while (last_tick < ev->getTick()) {
                        last_tick += getEchoResolution();
                        sendEchoEvent(last_tick);
                    }
                }*/
                if (!stopRequested() && !SequencerEvent::isConnectionChange(ev)) {
                    if (last_tick != ev->getTick()) {
                        last_tick = ev->getTick();
                        sendEchoEvent(last_tick);
                    }
                    if (!m_song->mutedState(ev->getTag())) {
                        SequencerEvent* ev2 = ev->clone();
                        ev2->setSource(m_PortId);
                        sendSongEvent(ev2);
                        delete ev2;
                    }
                }
                if (!stopRequested() && !hasNext()) {
                    if (final_tick > last_tick)
                        sendEchoEvent(final_tick);
                    if (!stopRequested() && m_loop) {
                        drainOutput();
                        syncOutput();
                        resetPosition();
                        last_tick = 0;
                        m_Queue->setTickPosition(0);
                    }
                }
            }
            if (stopRequested()) {
                m_Queue->clear();
                emit stopped();
            } else {
                drainOutput();
                syncOutput();
                if (stopRequested())
                    emit stopped();
                else
                    emit finished();
            }
            m_Queue->stop();
        } catch (...) {
            qWarning("exception in output thread");
        }
        m_npfds = 0;
        m_pfds = 0;
    }
}

void Player::setLoop(bool enabled)
{
    m_loop = enabled;
}
