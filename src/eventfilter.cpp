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

#include "eventfilter.h"

QString CategoryFilter::getName(int t)
{
    if (m_actions.contains(t))
        return m_actions[t]->text().replace("&", "");
    return QString();
}

bool CategoryFilter::getFilter(int t) const
{
    if (m_actions.contains(t))
        return m_filter & m_actions[t]->isChecked();
    return true;
}

void CategoryFilter::setFilter(int t, bool value)
{
    if (m_actions.contains(t))
        m_actions[t]->setChecked(value);
}

void CategoryFilter::insert(QObject* parent, snd_seq_event_type_t t, QString s)
{
    KToggleAction* a = new KToggleAction(parent);
    a->setText(s);
    a->setChecked(true);
    m_actions.insert((int)t, a);
}

EventFilter::EventFilter(QObject* parent)
    : QObject(parent)
{
    m_cats.insert(ChannelCategory, CategoryFilter(i18n("MIDI Channel")));
    m_cats.insert(SysCommonCategory, CategoryFilter(i18n("MIDI System Common")));
    m_cats.insert(SysRTCategory, CategoryFilter(i18n("MIDI System Real-Time")));
    m_cats.insert(SysExCategory, CategoryFilter(i18n("MIDI System Exclusive")));
    m_cats.insert(ALSACategory, CategoryFilter(i18n("ALSA")));
    m_cats.insert(SMFCategory, CategoryFilter(i18n("SMF")));
    /* MIDI Channel events */
    insert(ChannelCategory, SND_SEQ_EVENT_NOTEON, i18n("Note on"));
    insert(ChannelCategory, SND_SEQ_EVENT_NOTEOFF, i18n("Note off"));
    insert(ChannelCategory, SND_SEQ_EVENT_KEYPRESS, i18n("Polyphonic aftertouch"));
    insert(ChannelCategory, SND_SEQ_EVENT_CONTROLLER, i18n("Control change"));
    insert(ChannelCategory, SND_SEQ_EVENT_PGMCHANGE, i18n("Program change"));
    insert(ChannelCategory, SND_SEQ_EVENT_CHANPRESS, i18n("Channel aftertouch"));
    insert(ChannelCategory, SND_SEQ_EVENT_PITCHBEND, i18n("Pitch bend"));
    insert(ChannelCategory, SND_SEQ_EVENT_CONTROL14, i18n("Control change"));
    insert(ChannelCategory, SND_SEQ_EVENT_NONREGPARAM, i18n("Non-registered parameter"));
    insert(ChannelCategory, SND_SEQ_EVENT_REGPARAM, i18n("Registered parameter"));
    /* MIDI System exclusive events */
    insert(SysExCategory, SND_SEQ_EVENT_SYSEX, i18n("System exclusive"));
    /* MIDI Common events */
    insert(SysCommonCategory, SND_SEQ_EVENT_SONGPOS, i18n("Song Position"));
    insert(SysCommonCategory, SND_SEQ_EVENT_SONGSEL, i18n("Song Selection"));
    insert(SysCommonCategory, SND_SEQ_EVENT_QFRAME, i18n("MTC Quarter Frame"));
    insert(SysCommonCategory, SND_SEQ_EVENT_TUNE_REQUEST, i18n("Tune Request"));
    /* MIDI Realtime Events */
    insert(SysRTCategory, SND_SEQ_EVENT_START, i18n("Start"));
    insert(SysRTCategory, SND_SEQ_EVENT_CONTINUE, i18n("Continue"));
    insert(SysRTCategory, SND_SEQ_EVENT_STOP, i18n("Stop"));
    insert(SysRTCategory, SND_SEQ_EVENT_CLOCK, i18n("Clock"));
    insert(SysRTCategory, SND_SEQ_EVENT_TICK, i18n("Tick"));
    insert(SysRTCategory, SND_SEQ_EVENT_RESET, i18n("Reset"));
    insert(SysRTCategory, SND_SEQ_EVENT_SENSING, i18n("Active Sensing"));
    /* ALSA Client/Port events */
    insert(ALSACategory, SND_SEQ_EVENT_PORT_START, i18n("ALSA Port start"));
    insert(ALSACategory, SND_SEQ_EVENT_PORT_EXIT, i18n("ALSA Port exit"));
    insert(ALSACategory, SND_SEQ_EVENT_PORT_CHANGE, i18n("ALSA Port change"));
    insert(ALSACategory, SND_SEQ_EVENT_CLIENT_START, i18n("ALSA Client start"));
    insert(ALSACategory, SND_SEQ_EVENT_CLIENT_EXIT, i18n("ALSA Client exit"));
    insert(ALSACategory, SND_SEQ_EVENT_CLIENT_CHANGE, i18n("ALSA Client change"));
    insert(ALSACategory, SND_SEQ_EVENT_PORT_SUBSCRIBED, i18n("ALSA Port subscribed"));
    insert(ALSACategory, SND_SEQ_EVENT_PORT_UNSUBSCRIBED, i18n("ALSA Port unsubscribed"));
    /* SMF events */
    insert(SMFCategory, SND_SEQ_EVENT_TEMPO, i18n("Tempo"));
    insert(SMFCategory, SND_SEQ_EVENT_USR_VAR0, i18n("SMF Text"));
    insert(SMFCategory, SND_SEQ_EVENT_TIMESIGN, i18n("Time Signature"));
    insert(SMFCategory, SND_SEQ_EVENT_KEYSIGN, i18n("Key Signature"));
}

QString EventFilter::getName(EvCategory c)
{
    if (m_cats.contains(c)) {
        return m_cats[c].getName();
    }
    return QString();
}

bool EventFilter::getFilter(EvCategory c) const
{
    if (m_cats.contains(c)) {
        return m_cats[c].getFilter();
    }
    return true;
}

void EventFilter::setFilter(EvCategory c, bool value)
{
    if (m_cats.contains(c)) {
        m_cats[c].setFilter(value);
    }
}

QString EventFilter::getName(snd_seq_event_type_t t)
{
    if (m_aux.contains(t)) {
        EvCategory c = m_aux[t];
        return m_cats[c].getName(t);
    }
    return QString();
}

bool EventFilter::getFilter(snd_seq_event_type_t t) const
{
    if (m_aux.contains(t)) {
        EvCategory c = m_aux[t];
        return m_cats[c].getFilter(t);
    }
    return true;
}

bool EventFilter::contains(snd_seq_event_type_t t) const
{
    return m_aux.contains(t);
}

QMenu* EventFilter::buildMenu(QWidget* parent)
{
    QMenu *menu = new QMenu(parent);
    menu->setTitle(i18n("Filters"));
    foreach( CategoryFilter cf, m_cats ) {
        QMenu* submenu = new QMenu(parent);
        submenu->setTitle(cf.getName());
        menu->addMenu(submenu);
        QHashIterator<int, KToggleAction*> it = cf.getIterator();
        while( it.hasNext() ) {
            it.next();
            KToggleAction *item = it.value();
            connect(item, SIGNAL(triggered()), SIGNAL(filterChanged()));
            submenu->addAction( item );
        }
    }
    return menu;
}

void EventFilter::setFilter(snd_seq_event_type_t t, bool value)
{
    if (m_aux.contains(t)) {
        EvCategory c = m_aux[t];
        m_cats[c].setFilter(t, value);
    }
}

void EventFilter::insert(EvCategory category, snd_seq_event_type_t t, QString name)
{
    m_cats[category].insert(this, t, name);
    m_aux.insert(t, category);
}
