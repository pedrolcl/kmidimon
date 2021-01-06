/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2021 Pedro Lopez-Cabanillas                        *
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

#include "eventfilter.h"
//#include <QDebug>
#include <QSettings>
#include <QApplication>

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

void CategoryFilter::insert(QObject* parent, snd_seq_event_type_t t)
{
    QAction* a = new QAction(parent);
    a->setText(CategoryFilter::nameOfEvent(t));
    a->setCheckable(true);
    a->setChecked(true);
    a->setData((int)t);
    m_actions.insert((int)t, a);
}

QString CategoryFilter::nameOfEvent(int t)
{
    QHash<int, QString> names {
        /* MIDI Channel events */
        {SND_SEQ_EVENT_NOTE, QApplication::tr("Note")},
        {SND_SEQ_EVENT_NOTEON, QApplication::tr("Note on")},
        {SND_SEQ_EVENT_NOTEOFF, QApplication::tr("Note off")},
        {SND_SEQ_EVENT_KEYPRESS, QApplication::tr("Polyphonic aftertouch")},
        {SND_SEQ_EVENT_CONTROLLER, QApplication::tr("Control change")},
        {SND_SEQ_EVENT_PGMCHANGE, QApplication::tr("Program change")},
        {SND_SEQ_EVENT_CHANPRESS, QApplication::tr("Channel aftertouch")},
        {SND_SEQ_EVENT_PITCHBEND, QApplication::tr("Pitch bend")},
        {SND_SEQ_EVENT_CONTROL14, QApplication::tr("Control change")},
        {SND_SEQ_EVENT_NONREGPARAM, QApplication::tr("Non-registered parameter")},
        {SND_SEQ_EVENT_REGPARAM, QApplication::tr("Registered parameter")},
        /* MIDI System exclusive events */
        {SND_SEQ_EVENT_SYSEX, QApplication::tr("System exclusive")},
        /* MIDI Common events */
        {SND_SEQ_EVENT_SONGPOS, QApplication::tr("Song Position")},
        {SND_SEQ_EVENT_SONGSEL, QApplication::tr("Song Selection")},
        {SND_SEQ_EVENT_QFRAME, QApplication::tr("MTC Quarter Frame")},
        {SND_SEQ_EVENT_TUNE_REQUEST, QApplication::tr("Tune Request")},
        /* MIDI Realtime Events */
        {SND_SEQ_EVENT_START, QApplication::tr("Start", "player start")},
        {SND_SEQ_EVENT_CONTINUE, QApplication::tr("Continue")},
        {SND_SEQ_EVENT_STOP, QApplication::tr("Stop")},
        {SND_SEQ_EVENT_CLOCK, QApplication::tr("Clock")},
        {SND_SEQ_EVENT_TICK, QApplication::tr("Tick")},
        {SND_SEQ_EVENT_RESET, QApplication::tr("Reset")},
        {SND_SEQ_EVENT_SENSING, QApplication::tr("Active Sensing")},
        /* ALSA Client/Port events */
        {SND_SEQ_EVENT_PORT_START, QApplication::tr("ALSA Port start")},
        {SND_SEQ_EVENT_PORT_EXIT, QApplication::tr("ALSA Port exit")},
        {SND_SEQ_EVENT_PORT_CHANGE, QApplication::tr("ALSA Port change")},
        {SND_SEQ_EVENT_CLIENT_START, QApplication::tr("ALSA Client start")},
        {SND_SEQ_EVENT_CLIENT_EXIT, QApplication::tr("ALSA Client exit")},
        {SND_SEQ_EVENT_CLIENT_CHANGE, QApplication::tr("ALSA Client change")},
        {SND_SEQ_EVENT_PORT_SUBSCRIBED, QApplication::tr("ALSA Port subscribed")},
        {SND_SEQ_EVENT_PORT_UNSUBSCRIBED, QApplication::tr("ALSA Port unsubscribed")},
        /* SMF events */
        {SND_SEQ_EVENT_TEMPO, QApplication::tr("Tempo")},
        {SND_SEQ_EVENT_USR_VAR0, QApplication::tr("SMF Text")},
        {SND_SEQ_EVENT_TIMESIGN, QApplication::tr("Time Signature")},
        {SND_SEQ_EVENT_KEYSIGN, QApplication::tr("Key Signature")},
        {SND_SEQ_EVENT_USR1, QApplication::tr("Sequence Number")},
        {SND_SEQ_EVENT_USR2, QApplication::tr("Forced Channel")},
        {SND_SEQ_EVENT_USR3, QApplication::tr("Forced Port")},
        {SND_SEQ_EVENT_USR4, QApplication::tr("SMPTE Offset")},
        {SND_SEQ_EVENT_USR_VAR1, QApplication::tr("Sequencer Specific")},
        {SND_SEQ_EVENT_USR_VAR2, QApplication::tr("Meta (unregistered)")}
    };
    if (names.contains(t)) {
        return names[t];
    }
    return QString();
}

EventFilter::EventFilter(QObject* parent)
    : QObject(parent), m_menu(nullptr)
{
    m_cats.insert(ChannelCategory, new CategoryFilter(tr("MIDI Channel")));
    m_cats.insert(SysCommonCategory, new CategoryFilter(tr("MIDI System Common")));
    m_cats.insert(SysRTCategory, new CategoryFilter(tr("MIDI System Real-Time")));
    m_cats.insert(SysExCategory, new CategoryFilter(tr("MIDI System Exclusive")));
    m_cats.insert(ALSACategory, new CategoryFilter(tr("ALSA")));
    m_cats.insert(SMFCategory, new CategoryFilter(tr("SMF")));
    /* MIDI Channel events */
    insert(ChannelCategory, SND_SEQ_EVENT_NOTE);// tr("Note"));
    insert(ChannelCategory, SND_SEQ_EVENT_NOTEON);// tr("Note on"));
    insert(ChannelCategory, SND_SEQ_EVENT_NOTEOFF);// tr("Note off"));
    insert(ChannelCategory, SND_SEQ_EVENT_KEYPRESS);// tr("Polyphonic aftertouch"));
    insert(ChannelCategory, SND_SEQ_EVENT_CONTROLLER);// tr("Control change"));
    insert(ChannelCategory, SND_SEQ_EVENT_PGMCHANGE);//tr("Program change"));
    insert(ChannelCategory, SND_SEQ_EVENT_CHANPRESS);// tr("Channel aftertouch"));
    insert(ChannelCategory, SND_SEQ_EVENT_PITCHBEND);// tr("Pitch bend"));
    insert(ChannelCategory, SND_SEQ_EVENT_CONTROL14);// tr("Control change"));
    insert(ChannelCategory, SND_SEQ_EVENT_NONREGPARAM);// tr("Non-registered parameter"));
    insert(ChannelCategory, SND_SEQ_EVENT_REGPARAM);// tr("Registered parameter"));
    /* MIDI System exclusive events */
    insert(SysExCategory, SND_SEQ_EVENT_SYSEX);// tr("System exclusive"));
    /* MIDI Common events */
    insert(SysCommonCategory, SND_SEQ_EVENT_SONGPOS);// tr("Song Position"));
    insert(SysCommonCategory, SND_SEQ_EVENT_SONGSEL);//tr("Song Selection"));
    insert(SysCommonCategory, SND_SEQ_EVENT_QFRAME);// tr("MTC Quarter Frame"));
    insert(SysCommonCategory, SND_SEQ_EVENT_TUNE_REQUEST);// tr("Tune Request"));
    /* MIDI Realtime Events */
    insert(SysRTCategory, SND_SEQ_EVENT_START);// tr("Start", "player start"));
    insert(SysRTCategory, SND_SEQ_EVENT_CONTINUE);// tr("Continue"));
    insert(SysRTCategory, SND_SEQ_EVENT_STOP);// tr("Stop"));
    insert(SysRTCategory, SND_SEQ_EVENT_CLOCK);// tr("Clock"));
    insert(SysRTCategory, SND_SEQ_EVENT_TICK);// tr("Tick"));
    insert(SysRTCategory, SND_SEQ_EVENT_RESET);// tr("Reset"));
    insert(SysRTCategory, SND_SEQ_EVENT_SENSING);// tr("Active Sensing"));
    /* ALSA Client/Port events */
    insert(ALSACategory, SND_SEQ_EVENT_PORT_START);// tr("ALSA Port start"));
    insert(ALSACategory, SND_SEQ_EVENT_PORT_EXIT);// tr("ALSA Port exit"));
    insert(ALSACategory, SND_SEQ_EVENT_PORT_CHANGE);// tr("ALSA Port change"));
    insert(ALSACategory, SND_SEQ_EVENT_CLIENT_START);// tr("ALSA Client start"));
    insert(ALSACategory, SND_SEQ_EVENT_CLIENT_EXIT);// tr("ALSA Client exit"));
    insert(ALSACategory, SND_SEQ_EVENT_CLIENT_CHANGE);// tr("ALSA Client change"));
    insert(ALSACategory, SND_SEQ_EVENT_PORT_SUBSCRIBED);// tr("ALSA Port subscribed"));
    insert(ALSACategory, SND_SEQ_EVENT_PORT_UNSUBSCRIBED);// tr("ALSA Port unsubscribed"));
    /* SMF events */
    insert(SMFCategory, SND_SEQ_EVENT_TEMPO);// tr("Tempo"));
    insert(SMFCategory, SND_SEQ_EVENT_USR_VAR0);// tr("SMF Text"));
    insert(SMFCategory, SND_SEQ_EVENT_TIMESIGN);// tr("Time Signature"));
    insert(SMFCategory, SND_SEQ_EVENT_KEYSIGN);// tr("Key Signature"));
    insert(SMFCategory, SND_SEQ_EVENT_USR1);// tr("Sequence Number"));
    insert(SMFCategory, SND_SEQ_EVENT_USR2);// tr("Forced Channel"));
    insert(SMFCategory, SND_SEQ_EVENT_USR3);// tr("Forced Port"));
    insert(SMFCategory, SND_SEQ_EVENT_USR4);// tr("SMPTE Offset"));
    insert(SMFCategory, SND_SEQ_EVENT_USR_VAR1);// tr("Sequencer Specific"));
    insert(SMFCategory, SND_SEQ_EVENT_USR_VAR2);// tr("Meta (unregistered)"));
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
        if (m_cats[c]->getMenu() != nullptr)
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

void EventFilter::retranslateMenu()
{
    //qDebug() << Q_FUNC_INFO;
    if (m_menu != nullptr) {
        m_menu->setTitle(tr("Filters"));
        m_cats[ChannelCategory]->getMenu()->setTitle(tr("MIDI Channel"));
        m_cats[SysCommonCategory]->getMenu()->setTitle(tr("MIDI System Common"));
        m_cats[SysRTCategory]->getMenu()->setTitle(tr("MIDI System Real-Time"));
        m_cats[SysExCategory]->getMenu()->setTitle(tr("MIDI System Exclusive"));
        m_cats[ALSACategory]->getMenu()->setTitle(tr("ALSA"));
        m_cats[SMFCategory]->getMenu()->setTitle(tr("SMF"));
        foreach( CategoryFilter *cf, m_cats ) {
            auto ac = cf->getMenu()->actions();
            ac[0]->setText(tr("All", "check all types"));
            ac[1]->setText(tr("Nothing"));
            for(int i=3; i<ac.length(); ++i) {
                auto t = ac[i]->data().toInt();
                //qDebug() << i << ac[i]->text() << t;
                ac[i]->setText(CategoryFilter::nameOfEvent(t));
            }
        }
    }
}

QMenu* EventFilter::buildMenu(QWidget* parent)
{
    if (m_menu == nullptr) {
        m_menu = new QMenu(parent);
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
            connect(actionAll, &QAction::triggered, this, [=] { checkGroup(key); });
            submenu->addAction( actionAll );
            QAction *actionNothing = new QAction(tr("Nothing"), this);
            connect(actionNothing, &QAction::triggered, this, [=] { uncheckGroup(key); });
            submenu->addAction( actionNothing );
            submenu->addSeparator();
            QHashIterator<int, QAction*> it = cf->getIterator();
            while( it.hasNext() ) {
                it.next();
                QAction *item = it.value();
                connect(item, &QAction::triggered, this, &EventFilter::filterChanged);
                submenu->addAction( item );
            }
        }
        retranslateMenu();
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

void EventFilter::insert(EvCategory category, snd_seq_event_type_t t)
{
    m_cats[category]->insert(this, t);
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
