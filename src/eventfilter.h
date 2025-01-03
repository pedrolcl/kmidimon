/***************************************************************************
 *   Drumstick MIDI monitor based on the ALSA Sequencer                    *
 *   Copyright (C) 2005-2024 Pedro Lopez-Cabanillas                        *
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

#ifndef EVENTFILTER_H
#define EVENTFILTER_H

#include <QHash>
#include <QString>
#include <QMenu>
#include <QAction>

#include <drumstick/alsaevent.h>

enum EvCategory {
    ChannelCategory,
    SysCommonCategory,
    SysRTCategory,
    SysExCategory,
    ALSACategory,
    SMFCategory
};

class CategoryFilter {
public:
    CategoryFilter(QString name = QString()) : m_menu(nullptr), m_name(name), m_filter(true) {}
    virtual ~CategoryFilter() {}
    QString getName() const { return m_name; }
    QString getName(int t);
    QMenu* getMenu() const { return m_menu; }
    void setMenu(QMenu* mnu) { m_menu = mnu; }
    bool getFilter() const { return m_filter; }
    void setFilter(bool value) { m_filter = value; }
    bool getFilter(int t) const;
    void setFilter(int t, bool value);
    void insert(QObject* parent, snd_seq_event_type_t t);
    QHashIterator<int, QAction*> getIterator() {
        return QHashIterator<int, QAction*>(m_actions);
    }
    static QString nameOfEvent(int t);

private:
    QMenu  *m_menu;
    QString m_name;
    bool    m_filter;
    QHash<int, QAction*> m_actions;
};

class EventFilter : public QObject {
    Q_OBJECT
public:
    EventFilter(QObject* parent);
    virtual ~EventFilter();

    QString getName(EvCategory c);
    QString getName(snd_seq_event_type_t t);

    bool getFilter(EvCategory c) const;
    void setFilter(EvCategory c, bool value);
    bool getFilter(snd_seq_event_type_t t) const;
    void setFilter(snd_seq_event_type_t t, bool value);

    bool contains(snd_seq_event_type_t t) const;
    QMenu* buildMenu(QWidget* parent);
    void retranslateMenu();

    void loadConfiguration();
    void saveConfiguration();

public slots:
    void checkGroup(int c);
    void uncheckGroup(int c);

protected:
    void insert(EvCategory category, snd_seq_event_type_t t);

signals:
    void filterChanged();

private:
    QMenu *m_menu;
    QHash<EvCategory, CategoryFilter*> m_cats;
    QHash<int, EvCategory> m_aux;
};

#endif /* EVENTFILTER_H */
