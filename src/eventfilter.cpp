/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2019 Pedro Lopez-Cabanillas                        *
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
#include <QSettings>

QString CategoryFilter::getName(int t)
{
    if (m_actions.contains(t))
        return m_actions[t]->text().remove('&');
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
    QAction* a = new QAction(parent);
    a->setText(s);
    a->setCheckable(true);
    a->setChecked(true);
    m_actions.insert((int)t, a);
}

EventFilter::EventFilter(QObject* parent)
    : QObject(parent), m_menu(NULL)
{
    m_cats.insert(ChannelCategory, new CategoryFilter(tr("MIDI Channel")));
    m_cats.insert(SysCommonCategory, new CategoryFilter(tr("MIDI System Common")));
    m_cats.insert(SysRTCategory, new CategoryFilter(tr("MIDI System Real-Time")));
    m_cats.insert(SysExCategory, new CategoryFilter(tr("MIDI System Exclusive")));
    m_cats.insert(ALSACategory, new CategoryFilter(tr("ALSA")));
    m_cats.insert(SMFCategory, new CategoryFilter(tr("SMF")));
    /* MIDI Channel events */
    insert(ChannelCategory, SND_SEQ_EVENT_NOTE, tr("Note"));
    insert(ChannelCategory, SND_SEQ_EVENT_NOTEON, tr("Note on"));
    insert(ChannelCategory, SND_SEQ_EVENT_NOTEOFF, tr("Note off"));
    insert(ChannelCategory, SND_SEQ_EVENT_KEYPRESS, tr("Polyphonic aftertouch"));
    insert(ChannelCategory, SND_SEQ_EVENT_CONTROLLER, tr("Control change"));
    insert(ChannelCategory, SND_SEQ_EVENT_PGMCHANGE, tr("Program change"));
    insert(ChannelCategory, SND_SEQ_EVENT_CHANPRESS, tr("Channel aftertouch"));
    insert(ChannelCategory, SND_SEQ_EVENT_PITCHBEND, tr("Pitch bend"));
    insert(ChannelCategory, SND_SEQ_EVENT_CONTROL14, tr("Control change"));
    insert(ChannelCategory, SND_SEQ_EVENT_NONREGPARAM, tr("Non-registered parameter"));
    insert(ChannelCategory, SND_SEQ_EVENT_REGPARAM, tr("Registered parameter"));
    /* MIDI System exclusive events */
    insert(SysExCategory, SND_SEQ_EVENT_SYSEX, tr("System exclusive"));
    /* MIDI Common events */
    insert(SysCommonCategory, SND_SEQ_EVENT_SONGPOS, tr("Song Position"));
    insert(SysCommonCategory, SND_SEQ_EVENT_SONGSEL, tr("Song Selection"));
    insert(SysCommonCategory, SND_SEQ_EVENT_QFRAME, tr("MTC Quarter Frame"));
    insert(SysCommonCategory, SND_SEQ_EVENT_TUNE_REQUEST, tr("Tune Request"));
    /* MIDI Realtime Events */
    insert(SysRTCategory, SND_SEQ_EVENT_START, tr("Start", "player start"));
    insert(SysRTCategory, SND_SEQ_EVENT_CONTINUE, tr("Continue"));
    insert(SysRTCategory, SND_SEQ_EVENT_STOP, tr("Stop"));
    insert(SysRTCategory, SND_SEQ_EVENT_CLOCK, tr("Clock"));
    insert(SysRTCategory, SND_SEQ_EVENT_TICK, tr("Tick"));
    insert(SysRTCategory, SND_SEQ_EVENT_RESET, tr("Reset"));
    insert(SysRTCategory, SND_SEQ_EVENT_SENSING, tr("Active Sensing"));
    /* ALSA Client/Port events */
    insert(ALSACategory, SND_SEQ_EVENT_PORT_START, tr("ALSA Port start"));
    insert(ALSACategory, SND_SEQ_EVENT_PORT_EXIT, tr("ALSA Port exit"));
    insert(ALSACategory, SND_SEQ_EVENT_PORT_CHANGE, tr("ALSA Port change"));
    insert(ALSACategory, SND_SEQ_EVENT_CLIENT_START, tr("ALSA Client start"));
    insert(ALSACategory, SND_SEQ_EVENT_CLIENT_EXIT, tr("ALSA Client exit"));
    insert(ALSACategory, SND_SEQ_EVENT_CLIENT_CHANGE, tr("ALSA Client change"));
    insert(ALSACategory, SND_SEQ_EVENT_PORT_SUBSCRIBED, tr("ALSA Port subscribed"));
    insert(ALSACategory, SND_SEQ_EVENT_PORT_UNSUBSCRIBED, tr("ALSA Port unsubscribed"));
    /* SMF events */
    insert(SMFCategory, SND_SEQ_EVENT_TEMPO, tr("Tempo"));
    insert(SMFCategory, SND_SEQ_EVENT_USR_VAR0, tr("SMF Text"));
    insert(SMFCategory, SND_SEQ_EVENT_TIMESIGN, tr("Time Signature"));
    insert(SMFCategory, SND_SEQ_EVENT_KEYSIGN, tr("Key Signature"));
    insert(SMFCategory, SND_SEQ_EVENT_USR1, tr("Sequence Number"));
    insert(SMFCategory, SND_SEQ_EVENT_USR2, tr("Forced Channel"));
    insert(SMFCategory, SND_SEQ_EVENT_USR3, tr("Forced Port"));
    insert(SMFCategory, SND_SEQ_EVENT_USR4, tr("SMPTE Offset"));
    insert(SMFCategory, SND_SEQ_EVENT_USR_VAR1, tr("Sequencer Specific"));
    insert(SMFCategory, SND_SEQ_EVENT_USR_VAR2, tr("Meta (unregistered)"));
}

void EventFilter::checkGroup(int c)
{
    EvCategory cat = (EvCategory) c;
    QHashIterator<int, QAction*> it = m_cats[cat]->getIterator();
    while( it.hasNext() ) {
        it.next();
        QAction *item = it.value();
        item->setChecked(true);
    }
    emit filterChanged();
}

void EventFilter::uncheckGroup(int c)
{
    EvCategory cat = (EvCategory) c;
    QHashIterator<int, QAction*> it = m_cats[cat]->getIterator();
    while( it.hasNext() ) {
        it.next();
        QAction *item = it.value();
        item->setChecked(false);
    }
    emit filterChanged();
}

QString EventFilter::getName(EvCategory c)
{
    if (m_cats.contains(c)) {
        return m_cats[c]->getName();
    }
    return QString();
}

bool EventFilter::getFilter(EvCategory c) const
{
    if (m_cats.contains(c)) {
        return m_cats[c]->getFilter();
    }
    return true;
}

void EventFilter::setFilter(EvCategory c, bool value)
{
    if (m_cats.contains(c)) {
        m_cats[c]->setFilter(value);
        if (m_cats[c]->getMenu() != NULL)
            m_cats[c]->getMenu()->setEnabled(value);
    }
}

QString EventFilter::getName(snd_seq_event_type_t t)
{
    if (m_aux.contains(t)) {
        EvCategory c = m_aux[t];
        return m_cats[c]->getName(t);
    }
    return QString();
}

bool EventFilter::getFilter(snd_seq_event_type_t t) const
{
    if (m_aux.contains(t)) {
        EvCategory c = m_aux[t];
        return m_cats[c]->getFilter(t);
    }
    return true;
}

bool EventFilter::contains(snd_seq_event_type_t t) const
{
    return m_aux.contains(t);
}

QMenu* EventFilter::buildMenu(QWidget* parent)
{
    if (m_menu == NULL) {
        m_menu = new QMenu(parent);
        m_menu->setTitle(tr("Filters"));
        QHashIterator<EvCategory, CategoryFilter*> iter(m_cats);
        while ( iter.hasNext() ) {
            iter.next();
            int key = (int) iter.key();
            CategoryFilter *cf = iter.value();
            QMenu* submenu = new QMenu(parent);
            submenu->setTitle(cf->getName());
            m_menu->addMenu(submenu);
            cf->setMenu(submenu);
            QAction *actionAll = new QAction(tr("All", "check all types"), this);
            connect(actionAll, &QAction::triggered, [=] { checkGroup(key); });
            submenu->addAction( actionAll );
            QAction *actionNothing = new QAction(tr("Nothing"), this);
            connect(actionNothing, &QAction::triggered, [=] { uncheckGroup(key); });
            submenu->addAction( actionNothing );
            submenu->addSeparator();
            QHashIterator<int, QAction*> it = cf->getIterator();
            while( it.hasNext() ) {
                it.next();
                QAction *item = it.value();
                connect(item, SIGNAL(triggered()), SIGNAL(filterChanged()));
                submenu->addAction( item );
            }
        }
    }
    return m_menu;
}

void EventFilter::setFilter(snd_seq_event_type_t t, bool value)
{
    if (m_aux.contains(t)) {
        EvCategory c = m_aux[t];
        m_cats[c]->setFilter(t, value);
    }
}

void EventFilter::insert(EvCategory category, snd_seq_event_type_t t, QString name)
{
    m_cats[category]->insert(this, t, name);
    m_aux.insert(t, category);
}

void EventFilter::loadConfiguration()
{
    QSettings config;
    config.beginGroup("Filters");
    foreach( CategoryFilter *cf, m_cats ) {
        QHashIterator<int, QAction*> it = cf->getIterator();
        while( it.hasNext() ) {
            it.next();
            QAction *item = it.value();
            QString fkey = QString("filter_%1").arg(it.key());
            item->setChecked( config.value(fkey, true).toBool() );
        }
    }
}

void EventFilter::saveConfiguration()
{
    QSettings config;
    config.beginGroup("Filters");
    foreach( CategoryFilter *cf, m_cats ) {
        QHashIterator<int, QAction*> it = cf->getIterator();
        while( it.hasNext() ) {
            it.next();
            QAction *item = it.value();
            QString fkey = QString("filter_%1").arg(it.key());
            config.setValue( fkey, item->isChecked() );
        }
    }
    config.sync();
}
