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

#ifndef _KMIDIMON_H_
#define _KMIDIMON_H_

#include <kxmlguiwindow.h>
#include "sequenceradaptor.h"

class KAction;
class KToggleAction;
class QMenu;
class QEvent;
class QContextMenuEvent;
class QTreeView;
class QModelIndex;
class QSignalMapper;
class SequenceModel;
class QTabBar;
class ProxyModel;

const int COLUMN_COUNT = 7;

class KMidimon : public KXmlGuiWindow
{
    Q_OBJECT
public:
    KMidimon();
    virtual ~KMidimon() {}
    bool queryExit();

public slots:
    void fileNew();
    void fileSave();
    void preferences();
    void record();
    void stop();
    void connectAll();
    void addTrack();
    void deleteTrack();
    void changeTrack();

    void disconnectAll();
    void configConnections();
    void updateState(const QString newState);
    void editToolbars();
    void contextMenuEvent( QContextMenuEvent *ev );
    void setColumnStatus(int colNum, bool status);
    void toggleColumn(int colNum);
    void resizeColumns(const QModelIndex& parent, int start, int end);
    void tabIndexChanged(int index);

protected:
    void saveConfiguration();
    void readConfiguration();
    void setupActions();
    void setFixedFont(bool newValue);
    bool getFixedFont() const { return m_useFixedFont; }
    void setOrderedEvents(bool newValue);
    bool orderedEvents() const { return m_orderedEvents; }
    void addNewTab(int data);

private:
    SequencerAdaptor *m_adaptor;
    KAction *m_stop;
    KAction *m_record;
    KAction *m_prefs;
    KAction *m_save;
    KAction *m_connectAll;
    KAction *m_disconnectAll;
    KAction *m_configConns;
    KAction *m_createTrack;
    KAction *m_deleteTrack;
    KAction *m_changeTrack;
    KToggleAction *m_popupAction[COLUMN_COUNT];
    QMenu* m_popup;
    QTreeView* m_view;
    SequenceModel* m_model;
    ProxyModel* m_proxy;
    QSignalMapper* m_mapper;
    QTabBar* m_tabBar;
	bool m_useFixedFont;
	bool m_orderedEvents;
	int m_last_track;
};

#endif // _KMIDIMON_H_
