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

#ifndef KMIDIMON_H
#define KMIDIMON_H

#include <QString>
#include <QPointer>
#include <QMainWindow>
#include <QTranslator>

class QAction;
class QTabBar;
class QProgressDialog;
class QEvent;
class QContextMenuEvent;
class QTreeView;
class QModelIndex;
class QMenu;
class QSettings;
class QDir;
class QLabel;
class QComboBox;

class EventFilter;
class SequencerAdaptor;
class SequenceModel;
class ProxyModel;
class PlayerPopupSliderAction;
class HelpWindow;

const int COLUMN_COUNT = 8;
const int MaxRecentFiles = 10;

namespace Ui {
    class KMidimonWin;
}

class KMidimon : public QMainWindow
{
    Q_OBJECT

public:
    enum PlayerState {
        InvalidState,
        RecordingState,
        PlayingState,
        PausedState,
        StoppedState
    };
    Q_ENUM(PlayerState)

    KMidimon();
    virtual ~KMidimon();
    static QString dataDirectory();
    QString configuredLanguage();

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
    void closeEvent(QCloseEvent *event) override;
    void contextMenuEvent( QContextMenuEvent *ev ) override;
    void dropEvent( QDropEvent * event ) override;
    void dragEnterEvent( QDragEnterEvent * event ) override;

    void updateRecentFileActions();
    void openRecentFile();

protected:
    void saveConfiguration();
    void readConfiguration();
    void setupActions();
    void translateActions();
    void translateTabs();
    void setFixedFont(bool newValue);
    bool getFixedFont() const { return m_useFixedFont; }
    void addNewTab(int data);
    bool askTrackFilter(int& track);
    void updateCaption();
    void createLanguageMenu();
    void retranslateUi();
    void applyVisualStyle();
    void refreshIcons();

    void prependToRecentFiles(const QString &fileName);
    void setRecentFilesVisible(bool visible);
    bool hasRecentFiles();
    QStringList readRecentFiles(QSettings &settings);
    void writeRecentFiles(const QStringList &files, QSettings &settings);
    void initTextCodecs();
    void setTextCodec(const QString &encoding);
    void textCodecChanged(int index);

private:
    PlayerState m_state;
    SequencerAdaptor *m_adaptor;
    QAction *m_new;
    QAction *m_open;
    QAction *m_save;
    QAction *m_quit;
    QAction *m_play;
    QAction *m_pause;
    QAction *m_forward;
    QAction *m_rewind;
    QAction *m_stop;
    QAction *m_record;
    QAction *m_prefs;
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
    QAction *m_popupAction[COLUMN_COUNT];
    QMenu* m_popup;
    QTreeView* m_view;
    SequenceModel* m_model;
    ProxyModel* m_proxy;
    QTabBar* m_tabBar;
    QPointer<QProgressDialog> m_pd;
    QString m_outputConn;
    EventFilter* m_filter;
    QAction *m_muteTrack;
    bool m_useFixedFont;
    int m_defaultTempo;
    int m_defaultResolution;
    QString m_currentFile;
    QString m_currentState;
    bool m_autoResizeColumns;
    bool m_requestRealtimePrio;
    Ui::KMidimonWin *m_ui;
    QString m_language;
    QPointer<QTranslator> m_trp;
    QPointer<QTranslator> m_trq;
    QAction* m_currentLang;
    QMenu *m_recentFiles;
    QAction *m_recentFileActs[MaxRecentFiles];
    QAction *m_recentFileSeparator;
    QAction *m_recentFileSubMenuAct;
    QString m_style;
    bool m_darkMode;
    bool m_internalIcons;
    QPointer<HelpWindow> m_helpWindow;
    QMenu *m_menuTracks;
    QMenu *m_menuColumns;
    QLabel *m_combolbl;
    QComboBox *m_textcodecs;
};

#endif // KMIDIMON_H
