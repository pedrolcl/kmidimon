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

#ifndef KMIDIMON_H
#define KMIDIMON_H

#include <QPointer>
#include <kxmlguiwindow.h>

class KAction;
class KToggleAction;
class KRecentFilesAction;
class KTabBar;
class KProgressDialog;
class KUrl;
class EventFilter;

class QEvent;
class QContextMenuEvent;
class QTreeView;
class QModelIndex;
class QSignalMapper;
class QMenu;

class SequencerAdaptor;
class SequenceModel;
class ProxyModel;
class KPlayerPopupSliderAction;

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
    void fileOpen();
    void fileSave();
    void preferences();
    void record();
    void stop();
    void play();
    void pause();
    void rewind();
    void forward();
    void connectAll();
    void addTrack();
    void deleteCurrentTrack();
    void changeCurrentTrack();
    void deleteTrack(int tabIndex);
    void changeTrack(int tabIndex);
    void songFinished();
    void slotURLSelected(const KUrl& url);
    void tempoReset();
    void tempoSlider(int value);

    void disconnectAll();
    void configConnections();
    void updateState(const QString newState, const QString stateName);
    void editToolbars();
    void contextMenuEvent( QContextMenuEvent *ev );
    void setColumnStatus(int colNum, bool status);
    void toggleColumn(int colNum);
    void resizeColumns(const QModelIndex& parent, int start, int end);
    void resizeAllColumns();
    void tabIndexChanged(int index);
    void reorderTabs(int fromIndex, int toIndex);
    void slotTicks(int row);
    void slotCurrentChanged(const QModelIndex& curr, const QModelIndex& prev);
    void updateView();
    void songFileInfo();

protected:
    void saveConfiguration();
    void readConfiguration();
    void setupActions();
    void setFixedFont(bool newValue);
    bool getFixedFont() const { return m_useFixedFont; }
    void addNewTab(int data);
    bool askTrackFilter(int& track);

private:
    SequencerAdaptor *m_adaptor;
    KAction *m_play;
    KToggleAction *m_pause;
    KAction *m_forward;
    KAction *m_rewind;
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
    KAction *m_resizeColumns;
    KAction *m_fileInfo;
    KPlayerPopupSliderAction *m_tempoSlider;
    KAction *m_tempo100;
    KRecentFilesAction *m_recentFiles;
    KToggleAction *m_popupAction[COLUMN_COUNT];
    QMenu* m_popup;
    QTreeView* m_view;
    SequenceModel* m_model;
    ProxyModel* m_proxy;
    QSignalMapper* m_mapper;
    KTabBar* m_tabBar;
    QPointer<KProgressDialog> m_pd;
    QString m_outputConn;
    EventFilter* m_filter;
	bool m_useFixedFont;
    int m_defaultTempo;
    int m_defaultResolution;
    QString m_file;
};

#endif // KMIDIMON_H
