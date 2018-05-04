/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2013 Pedro Lopez-Cabanillas                        *
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

#include <QString>
#include <QPointer>
#include <QMainWindow>
#include <QTranslator>

class QAction;
class QTabBar;
class QProgressDialog;
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
class PlayerPopupSliderAction;

const int COLUMN_COUNT = 8;

namespace Ui {
class KMidimonWin;
}

enum PlayerState {
    InvalidState,
    RecordingState,
    PlayingState,
    PausedState,
    StoppedState
};

class KMidimon : public QMainWindow
{
    Q_OBJECT
public:
    KMidimon();
    virtual ~KMidimon();

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
    void muteCurrentTrack();
    void deleteTrack(int tabIndex);
    void changeTrack(int tabIndex);
    void muteTrack(int tabIndex);
    void songFinished();
    void open(const QString& fileName);
    void tempoReset();
    void tempoSlider(int value);
    void slotLoop();
    void about();
    void help();
    void slotOpenWebSite();

    void disconnectAll();
    void configConnections();
    void updateState(PlayerState newState);
    void setColumnStatus(int colNum, bool status);
    void toggleColumn(int colNum);
    void modelRowsInserted(const QModelIndex& parent, int start, int end);
    void resizeAllColumns();
    void tabIndexChanged(int index);
    void reorderTabs(int fromIndex, int toIndex);
    void slotTicks(int row);
    void slotCurrentChanged(const QModelIndex& curr, const QModelIndex& prev);
    void updateView();
    void songFileInfo();
    void slotSwitchLanguage(QAction *action);
    void closeEvent(QCloseEvent *event);
    void contextMenuEvent( QContextMenuEvent *ev );
    void dropEvent( QDropEvent * event );
    void dragEnterEvent( QDragEnterEvent * event );

protected:
    void saveConfiguration();
    void readConfiguration();
    void setupActions();
    void setFixedFont(bool newValue);
    bool getFixedFont() const { return m_useFixedFont; }
    void addNewTab(int data);
    bool askTrackFilter(int& track);
    void updateCaption();
    void createLanguageMenu();
    QString configuredLanguage();
    void retranslateUi();

private:
    PlayerState m_state;
    SequencerAdaptor *m_adaptor;
    QAction *m_play;
    QAction *m_pause;
    QAction *m_forward;
    QAction *m_rewind;
    QAction *m_stop;
    QAction *m_record;
    QAction *m_prefs;
    QAction *m_save;
    QAction *m_connectAll;
    QAction *m_disconnectAll;
    QAction *m_configConns;
    QAction *m_createTrack;
    QAction *m_deleteTrack;
    QAction *m_changeTrack;
    QAction *m_resizeColumns;
    QAction *m_fileInfo;
    PlayerPopupSliderAction *m_tempoSlider;
    QAction *m_tempo100;
    QMenu *m_recentFiles;
    QAction *m_popupAction[COLUMN_COUNT];
    QMenu* m_popup;
    QTreeView* m_view;
    SequenceModel* m_model;
    ProxyModel* m_proxy;
    QSignalMapper* m_mapper;
    QTabBar* m_tabBar;
    QPointer<QProgressDialog> m_pd;
    QString m_outputConn;
    EventFilter* m_filter;
    QAction *m_loop;
    QAction *m_muteTrack;
	bool m_useFixedFont;
    int m_defaultTempo;
    int m_defaultResolution;
    QString m_file;
    QString m_currentState;
    bool m_autoResizeColumns;
    bool m_requestRealtimePrio;
    Ui::KMidimonWin *m_ui;
    QString m_language;
    QTranslator* m_trp;
    QTranslator* m_trq;
    QAction* m_currentLang;
};

#endif // KMIDIMON_H
