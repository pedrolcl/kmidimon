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

#include <QMenu>
#include <QEvent>
#include <QContextMenuEvent>
#include <QCursor>
#include <QTreeView>
#include <QTextStream>
#include <QSignalMapper>
#include <QVariant>

#include <klocale.h>
#include <kaction.h>
#include <kapplication.h>
#include <kfiledialog.h>
#include <kedittoolbar.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <ktoggleaction.h>
#include <kstandardaction.h>
#include <kactioncollection.h>
#include <kxmlguifactory.h>
#include <kurl.h>
#include <ktabbar.h>
#include <kicon.h>
#include <kinputdialog.h>
#include <kprogressdialog.h>
#include <krecentfilesaction.h>
#include <kmenubar.h>

#include "kmidimon.h"
#include "configdialog.h"
#include "connectdlg.h"
#include "sequencemodel.h"
#include "proxymodel.h"
#include "eventfilter.h"

KMidimon::KMidimon() :
    KXmlGuiWindow(0)
{
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    m_useFixedFont = false;
    m_mapper = new QSignalMapper(this);
    m_model = new SequenceModel(this);
    m_proxy = new ProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_view = new QTreeView(this);
    m_view->setWhatsThis(i18n("The events list"));
    m_view->setRootIsDecorated(false);
    m_view->setAlternatingRowColors(true);
    m_view->setModel(m_proxy);
    m_view->setSortingEnabled(false);
    //QAbstractItemView::NoSelection
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    connect( m_view->selectionModel(),
             SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
             SLOT(slotCurrentChanged(const QModelIndex&, const QModelIndex&)) );
    m_adaptor = new SequencerAdaptor(this);
    m_adaptor->setModel(m_model);
    m_adaptor->updateModelClients();
    connect( m_model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                      SLOT(resizeColumns(QModelIndex,int,int)) );
    connect( m_adaptor, SIGNAL(signalTicks(int)), SLOT(slotTicks(int)));
    m_tabBar = new KTabBar(this);
    m_tabBar->setWhatsThis(i18n("Track view selectors"));
    m_tabBar->setShape(QTabBar::RoundedNorth);
#if QT_VERSION < 0x040500
    m_tabBar->setTabReorderingEnabled(true);
    m_tabBar->setCloseButtonEnabled(true);
    connect( m_tabBar, SIGNAL(moveTab(int,int)),
                       SLOT(reorderTabs(int,int)) );
    connect( m_tabBar, SIGNAL(closeRequest(int)),
                       SLOT(deleteTrack(int)) );
#else
    m_tabBar->setExpanding(false);
    m_tabBar->setMovable(true);
    m_tabBar->setTabsClosable(true);
    connect( m_tabBar, SIGNAL(tabCloseRequested(int)),
                       SLOT(deleteTrack(int)) );
#endif
    connect( m_tabBar, SIGNAL(newTabRequest()),
                       SLOT(addTrack()) );
    connect( m_tabBar, SIGNAL(tabDoubleClicked(int)),
                       SLOT(changeTrack(int)) );
    connect( m_tabBar, SIGNAL(currentChanged(int)),
                       SLOT(tabIndexChanged(int)) );
    layout->addWidget(m_tabBar);
    layout->addWidget(m_view);
    vbox->setLayout(layout);
    setCentralWidget(vbox);
    setupActions();
    setAutoSaveSettings();
    readConfiguration();
    fileNew();
    record();
}

void KMidimon::setupActions()
{
    const QString columnName[COLUMN_COUNT] = {
            i18n("Ticks"),
            i18n("Time"),
            i18n("Source"),
            i18n("Event Kind"),
            i18n("Channel"),
            i18n("Data 1"),
            i18n("Data 2")
    };
    const QString actionName[COLUMN_COUNT] = {
            "show_ticks",
            "show_time",
            "show_source",
            "show_kind",
            "show_channel",
            "show_data1",
            "show_data2"
    };
    KAction* a;

    a = KStandardAction::quit(kapp, SLOT(quit()), actionCollection());
    a->setWhatsThis(i18n("Exit the application"));
    a = KStandardAction::open(this, SLOT(fileOpen()), actionCollection());
    a->setWhatsThis(i18n("Open a disk file"));
    m_recentFiles = KStandardAction::openRecent(this, SLOT(slotURLSelected(const KUrl&)), actionCollection());
    a = KStandardAction::openNew(this, SLOT(fileNew()), actionCollection());
    a->setWhatsThis(i18n("Clear the current data and start a new empty session"));
    m_save = KStandardAction::saveAs(this, SLOT(fileSave()), actionCollection());
    m_save->setWhatsThis(i18n("Store the session data on a disk file"));
    m_prefs = KStandardAction::preferences(this, SLOT(preferences()), actionCollection());
    m_prefs->setWhatsThis(i18n("Configure the program setting several preferences"));
    a = KStandardAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());
    a->setWhatsThis(i18n("Organize the toolbar icons"));

    /* Icon naming specification
     * http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
     *
     * media-playback-pause     The icon for the pause action of a media player.
     * media-playback-start     The icon for the start playback action of a media player.
     * media-playback-stop      The icon for the stop action of a media player.
     * media-playlist-shuffle   The icon for the shuffle action of a media player.
     * media-record             The icon for the record action of a media application.
     * media-seek-backward      The icon for the seek backward action of a media player.
     * media-seek-forward       The icon for the seek forward action of a media player.
     * media-skip-backward      The icon for the skip backward action of a media player.
     * media-skip-forward       The icon for the skip forward action of a media player.
     */

    m_play = new KAction(this);
    m_play->setText(i18n("&Play"));
    m_play->setIcon(KIcon("media-playback-start"));
    m_play->setShortcut( Qt::Key_P );
    m_play->setWhatsThis(i18n("Start playback of the current session"));
    connect(m_play, SIGNAL(triggered()), SLOT(play()));
    actionCollection()->addAction("play", m_play);

    m_pause = new KToggleAction(this);
    m_pause->setText(i18n("Pause"));
    m_pause->setIcon(KIcon("media-playback-pause"));
    m_pause->setWhatsThis(i18n("Pause the playback"));
    connect(m_pause, SIGNAL(triggered()), SLOT(pause()));
    actionCollection()->addAction("pause", m_pause);

    m_forward = new KAction(this);
    m_forward->setText(i18n("Forward"));
    m_forward->setIcon(KIcon("media-skip-forward"));
    m_forward->setWhatsThis(i18n("Move the playback position to the last event"));
    connect(m_forward, SIGNAL(triggered()), SLOT(forward()));
    actionCollection()->addAction("forward", m_forward);

    m_rewind = new KAction(this);
    m_rewind->setText(i18n("Backward"));
    m_rewind->setIcon(KIcon("media-skip-backward"));
    m_rewind->setWhatsThis(i18n("Move the playback position to the first event"));
    connect(m_rewind, SIGNAL(triggered()), SLOT(rewind()));
    actionCollection()->addAction("rewind", m_rewind);

    m_record = new KAction(this);
    m_record->setText(i18n("&Record"));
    m_record->setIcon(KIcon("media-record"));
    m_record->setShortcut( Qt::Key_R );
    m_record->setWhatsThis(i18n("Append new recorded events to the current session"));
    connect(m_record, SIGNAL(triggered()), SLOT(record()));
    actionCollection()->addAction("record", m_record);

    m_stop = new KAction(this);
    m_stop->setText( i18n("&Stop") );
    m_stop->setIcon(KIcon("media-playback-stop"));
    m_stop->setShortcut( Qt::Key_S );
    m_stop->setWhatsThis(i18n("Stop playback or recording"));
    connect(m_stop, SIGNAL(triggered()), SLOT(stop()));
    actionCollection()->addAction("stop", m_stop);

    m_connectAll = new KAction(this);
    m_connectAll->setText(i18n("&Connect All Inputs"));
    m_connectAll->setWhatsThis(i18n("Connect all readable MIDI ports"));
    connect(m_connectAll, SIGNAL(triggered()), SLOT(connectAll()));
    actionCollection()->addAction("connect_all", m_connectAll);

    m_disconnectAll = new KAction(this);
    m_disconnectAll->setText(i18n("&Disconnect All Inputs"));
    m_disconnectAll->setWhatsThis(i18n("Disconnect all input MIDI ports"));
    connect(m_disconnectAll, SIGNAL(triggered()), SLOT(disconnectAll()));
    actionCollection()->addAction( "disconnect_all", m_disconnectAll );

    m_configConns = new KAction(this);
    m_configConns->setText(i18n("Con&figure Connections"));
    m_configConns->setWhatsThis(i18n("Open the Connections dialog"));
    connect(m_configConns, SIGNAL(triggered()), SLOT(configConnections()));
    actionCollection()->addAction("connections_dialog", m_configConns );

    m_createTrack = new KAction(this);
    m_createTrack->setText(i18n("&Add Track View"));
    m_createTrack->setWhatsThis(i18n("Create a new tab/track view"));
    connect(m_createTrack, SIGNAL(triggered()), SLOT(addTrack()));
    actionCollection()->addAction("add_track", m_createTrack );

    m_changeTrack = new KAction(this);
    m_changeTrack->setText(i18n("&Change Track View"));
    m_changeTrack->setWhatsThis(i18n("Change the track number of the view"));
    connect(m_changeTrack, SIGNAL(triggered()), SLOT(changeCurrentTrack()));
    actionCollection()->addAction("change_track", m_changeTrack );

    m_deleteTrack = new KAction(this);
    m_deleteTrack->setText(i18n("&Delete Track View"));
    m_deleteTrack->setWhatsThis(i18n("Delete the tab/track view"));
    connect(m_deleteTrack, SIGNAL(triggered()), SLOT(deleteCurrentTrack()));
    actionCollection()->addAction("delete_track", m_deleteTrack );

    for(int i = 0; i < COLUMN_COUNT; ++i ) {
        m_popupAction[i] = new KToggleAction(columnName[i], this);
        m_popupAction[i]->setWhatsThis(i18n("Toggle the %1 column",columnName[i]));
        connect(m_popupAction[i], SIGNAL(triggered()), m_mapper, SLOT(map()));
        m_mapper->setMapping(m_popupAction[i], i);
        actionCollection()->addAction(actionName[i], m_popupAction[i]);
    }
    connect(m_mapper, SIGNAL(mapped(int)), SLOT(toggleColumn(int)));

    m_resizeColumns = new KAction(this);
    m_resizeColumns->setText(i18n("&Resize columns"));
    m_resizeColumns->setWhatsThis(i18n("Resize the columns width to fit it's contents"));
    connect(m_resizeColumns, SIGNAL(triggered()), SLOT(resizeAllColumns()));
    actionCollection()->addAction("resize_columns", m_resizeColumns);

    setStandardToolBarMenuEnabled(true);
    setupGUI();

    m_popup = static_cast <QMenu*>(guiFactory()->container("popup", this));
    Q_CHECK_PTR( m_popup );

    m_filter = new EventFilter(this);
    QMenu* filtersMenu = m_filter->buildMenu(this);
    m_popup->addMenu( filtersMenu );
    m_model->setFilter(m_filter);
    m_proxy->setFilter(m_filter);
    connect(m_filter, SIGNAL(filterChanged()), m_proxy, SLOT(invalidate()));

    //menuBar()->addMenu( filtersMenu );
}

void KMidimon::fileNew()
{
    m_model->clear();
    for (int i = m_tabBar->count() - 1; i >= 0; i--) {
        m_tabBar->removeTab(i);
    }
    addNewTab(1);
    m_proxy->setFilterTrack(0);
    m_model->setCurrentTrack(0);
    m_model->setInitialTempo(m_defaultTempo);
    m_model->setDivision(m_defaultResolution);
    m_adaptor->setTempo(m_defaultTempo);
    m_adaptor->setResolution(m_defaultResolution);
    m_adaptor->queue_set_tempo();
    m_adaptor->rewind();
    updateView();
}

void KMidimon::slotURLSelected(const KUrl& url)
{
    QString path = url.toLocalFile();
    if (!path.isNull()) {
        QFileInfo finfo(path);
        if (finfo.exists()) {
            try {
                m_view->blockSignals(true);
                stop();
                m_model->clear();
                for (int i = m_tabBar->count() - 1; i >= 0; i--) {
                    m_tabBar->removeTab(i);
                }
                m_pd = new KProgressDialog(this, i18n("Load MIDI file"),
                                i18n("Loading..."));
                m_pd->setAllowCancel(false);
                m_pd->setMinimumDuration(500);
                m_pd->progressBar()->setRange(0, finfo.size());
                m_pd->progressBar()->setValue(0);
                connect( m_model, SIGNAL(loadProgress(int)),
                         m_pd->progressBar(), SLOT(setValue(int)) );
                m_model->loadFromFile(path);
                m_pd->progressBar()->setValue(finfo.size());
                m_adaptor->setResolution(m_model->getSMFDivision());
                if (m_model->getInitialTempo() > 0) {
                    m_adaptor->setTempo(m_model->getInitialTempo());
                    m_adaptor->queue_set_tempo();
                }
                m_adaptor->rewind();
                int ntrks = m_model->getSMFTracks();
                if (ntrks < 1) ntrks = 1;
                for (int i = 0; i < ntrks; i++)
                    addNewTab(i+1);
                m_tabBar->setCurrentIndex(0);
                m_proxy->setFilterTrack(0);
                m_model->setCurrentTrack(0);
                for (int i = 0; i < COLUMN_COUNT; ++i)
                    m_view->resizeColumnToContents(i);
            } catch (...) {
                m_model->clear();
            }
            m_view->blockSignals(false);
            m_recentFiles->addUrl(url);
            delete m_pd;
        }
    }
}

void KMidimon::fileOpen()
{
    KUrl u = KFileDialog::getOpenUrl(
            KUrl("kfiledialog:///MIDIMONITOR"),
            i18n("*.mid *.midi *.kar|MIDI files (*.mid)"),
            this,
            i18n("Open MIDI file"));
    if (!u.isEmpty()) slotURLSelected(u);
}

void KMidimon::fileSave()
{
    KUrl u = KFileDialog::getSaveUrl(
            KUrl("kfiledialog:///MIDIMONITOR"),
            i18n("*.txt|Plain text files (*.txt)\n"
                 "*.mid|MIDI files (*.mid)"),
            this,
            i18n("Save MIDI monitor data"));
    if (!u.isEmpty()) {
        QString path = u.toLocalFile();
        if (!path.isNull()) {
            m_model->saveToFile(path);
            m_recentFiles->addUrl(u);
        }
    }
}

bool KMidimon::queryExit()
{
    saveConfiguration();
    return true;
}

void KMidimon::saveConfiguration()
{
    int i;
    KConfigGroup config = KGlobal::config()->group("Settings");
    config.writeEntry("resolution", m_defaultResolution);
    config.writeEntry("tempo", m_defaultTempo);
    config.writeEntry("alsa", m_proxy->showAlsaMsg());
    config.writeEntry("channel", m_proxy->showChannelMsg());
    config.writeEntry("common", m_proxy->showCommonMsg());
    config.writeEntry("realtime", m_proxy->showRealTimeMsg());
    config.writeEntry("sysex", m_proxy->showSysexMsg());
    config.writeEntry("smf", m_proxy->showSmfMsg());
    config.writeEntry("client_names", m_model->showClientNames());
    config.writeEntry("translate_sysex", m_model->translateSysex());
    config.writeEntry("translate_notes", m_model->translateNotes());
    config.writeEntry("translate_ctrls", m_model->translateCtrls());
    config.writeEntry("instrument", m_model->getInstrumentName());
    config.writeEntry("fixed_font", getFixedFont());
    for (i = 0; i < COLUMN_COUNT; ++i) {
        config.writeEntry(QString("show_column_%1").arg(i),
                m_popupAction[i]->isChecked());
    }
    config.writeEntry("output_connection", m_outputConn);
    config.sync();

    config = KGlobal::config()->group("RecentFiles");
    m_recentFiles->saveEntries(config);
    config.sync();
}

void KMidimon::readConfiguration()
{
    int i;
    bool status;
    KConfigGroup config = KGlobal::config()->group("Settings");
    m_proxy->setFilterAlsaMsg(config.readEntry("alsa", true));
    m_proxy->setFilterChannelMsg(config.readEntry("channel", true));
    m_proxy->setFilterCommonMsg(config.readEntry("common", true));
    m_proxy->setFilterRealTimeMsg(config.readEntry("realtime", true));
    m_proxy->setFilterSysexMsg(config.readEntry("sysex", true));
    m_proxy->setFilterSmfMsg(config.readEntry("smf", true));
    m_model->setShowClientNames(config.readEntry("client_names", false));
    m_model->setTranslateSysex(config.readEntry("translate_sysex", false));
    m_model->setTranslateNotes(config.readEntry("translate_notes", false));
    m_model->setTranslateCtrls(config.readEntry("translate_ctrls", false));
    m_model->setInstrumentName(config.readEntry("instrument", QString()));
    m_adaptor->setResolution(m_defaultResolution = config.readEntry("resolution", RESOLUTION));
    m_adaptor->setTempo(m_defaultTempo = config.readEntry("tempo", TEMPO_BPM));
    m_adaptor->queue_set_tempo();
    setFixedFont(config.readEntry("fixed_font", false));
    for (i = 0; i < COLUMN_COUNT; ++i) {
        status = config.readEntry(QString("show_column_%1").arg(i), true);
        setColumnStatus(i, status);
    }
    m_outputConn = config.readEntry("output_connection", QString());
    m_adaptor->connect_output(m_outputConn);

    config = KGlobal::config()->group("RecentFiles");
    m_recentFiles->loadEntries(config);
}

void KMidimon::preferences()
{
    int i;
    bool was_running;
    ConfigDialog dlg;

    dlg.setTempo(m_defaultTempo);
    dlg.setResolution(m_defaultResolution);
    dlg.setRegAlsaMsg(m_proxy->showAlsaMsg());
    dlg.setRegChannelMsg(m_proxy->showChannelMsg());
    dlg.setRegCommonMsg(m_proxy->showCommonMsg());
    dlg.setRegRealTimeMsg(m_proxy->showRealTimeMsg());
    dlg.setRegSysexMsg(m_proxy->showSysexMsg());
    dlg.setRegSmfMsg(m_proxy->showSmfMsg());
    dlg.setShowClientNames(m_model->showClientNames());
    dlg.setTranslateSysex(m_model->translateSysex());
    dlg.setTranslateNotes(m_model->translateNotes());
    dlg.setTranslateCtrls(m_model->translateCtrls());
    dlg.setInstruments(m_model->getInstruments());
    dlg.setInstrumentName(m_model->getInstrumentName());
    dlg.setUseFixedFont(getFixedFont());
    for (i = 0; i < COLUMN_COUNT; ++i) {
        dlg.setShowColumn(i, m_popupAction[i]->isChecked());
    }
    if (dlg.exec()) {
        was_running = m_adaptor->isRecording();
        if (was_running) stop();
        m_proxy->setFilterAlsaMsg(dlg.isRegAlsaMsg());
        m_proxy->setFilterChannelMsg(dlg.isRegChannelMsg());
        m_proxy->setFilterCommonMsg(dlg.isRegCommonMsg());
        m_proxy->setFilterRealTimeMsg(dlg.isRegRealTimeMsg());
        m_proxy->setFilterSysexMsg(dlg.isRegSysexMsg());
        m_proxy->setFilterSmfMsg(dlg.isRegSmfMsg());
        m_model->setShowClientNames(dlg.showClientNames());
        m_model->setTranslateSysex(dlg.translateSysex());
        m_model->setTranslateNotes(dlg.translateNotes());
        m_model->setTranslateCtrls(dlg.translateCtrls());
        m_model->setInstrumentName(dlg.getInstrumentName());
        m_adaptor->setTempo(m_defaultTempo = dlg.getTempo());
        m_adaptor->setResolution(m_defaultResolution = dlg.getResolution());
        m_adaptor->queue_set_tempo();
        setFixedFont(dlg.useFixedFont());
        for (i = 0; i < COLUMN_COUNT; ++i) {
            setColumnStatus(i, dlg.showColumn(i));
        }
        if (was_running) record();
    }
}

void KMidimon::record()
{
    m_adaptor->forward();
    m_adaptor->record();
    updateState("recording_state", i18n("recording"));
}

void KMidimon::stop()
{
    if (m_adaptor->isRecording() | m_adaptor->isPlaying()) {
        m_adaptor->stop();
        songFinished();
    }
}

void KMidimon::songFinished()
{
    updateState("stopped_state", i18n("stopped"));
    updateView();
}

void KMidimon::play()
{
    m_adaptor->play();
    updateState("playing_state", i18n("playing"));
}

void KMidimon::pause()
{
    m_adaptor->pause(m_pause->isChecked());
}

void KMidimon::rewind()
{
    m_adaptor->rewind();
    updateView();
}

void KMidimon::forward()
{
    m_adaptor->forward();
    updateView();
}

void KMidimon::updateState(const QString newState, const QString stateName)
{
    setCaption(i18n("ALSA MIDI Monitor [%1]", stateName));
    slotStateChanged(newState);
}

void KMidimon::editToolbars()
{
    KEditToolBar dlg(actionCollection());
    if (dlg.exec()) {
        setupGUI();
    }
}

void KMidimon::connectAll()
{
    m_adaptor->connect_all_inputs();
}

void KMidimon::disconnectAll()
{
    m_adaptor->disconnect_all_inputs();
}

void KMidimon::configConnections()
{
    QStringList inputs = m_adaptor->inputConnections();
    QStringList subs = m_adaptor->list_subscribers();
    QStringList outputs = m_adaptor->outputConnections();
    m_outputConn = m_adaptor->output_subscriber();
    ConnectDlg dlg(this, inputs, subs, outputs, m_outputConn);
    if (dlg.exec()) {
        QStringList desired = dlg.getSelectedInputs();
        subs = m_adaptor->list_subscribers();
        QStringList::ConstIterator i;
        for (i = subs.constBegin(); i != subs.constEnd(); ++i) {
            if (desired.contains(*i) == 0) {
                m_adaptor->disconnect_input(*i);
            }
        }
        for (i = desired.constBegin(); i != desired.constEnd(); ++i) {
            if (subs.contains(*i) == 0) {
                m_adaptor->connect_input(*i);
            }
        }
        QString newOut = dlg.getSelectedOutput();
        if (newOut != m_outputConn) {
            m_adaptor->disconnect_output(m_outputConn);
            m_adaptor->connect_output(m_outputConn = newOut);
        }
    }
}

void KMidimon::setColumnStatus(int colNum, bool status)
{
    m_view->setColumnHidden(colNum, !status);
    m_popupAction[colNum]->setChecked(status);
    if (status) m_view->resizeColumnToContents(colNum);
}

void KMidimon::toggleColumn(int colNum)
{
    bool status = !m_popupAction[colNum]->isChecked();
    m_view->setColumnHidden(colNum, status);
    if (!status) m_view->resizeColumnToContents(colNum);
}

void KMidimon::contextMenuEvent(QContextMenuEvent*)
{
    Q_CHECK_PTR( m_popup );
    m_popup->exec(QCursor::pos());
}

void KMidimon::setFixedFont(bool newValue)
{
    if (m_useFixedFont != newValue) {
        m_useFixedFont = newValue;
        m_view->setFont(m_useFixedFont ? KGlobalSettings::fixedFont()
                : KGlobalSettings::generalFont());
    }
}

void KMidimon::resizeColumns(const QModelIndex&, int, int)
{
	resizeAllColumns();
    m_view->scrollToBottom();
}

void KMidimon::resizeAllColumns()
{
    for( int i = 0; i < COLUMN_COUNT; ++i)
        m_view->resizeColumnToContents(i);
}

void KMidimon::addNewTab(int data)
{
    QString tabName = i18n("Track %1", data);
    int i = m_tabBar->addTab(tabName);
    m_tabBar->setTabData(i, QVariant(data));
    m_tabBar->setTabWhatsThis(i, i18n("Track %1 View Selector", data));
    //qDebug() << "new tab data: " << data;
}

void KMidimon::tabIndexChanged(int index)
{
    QVariant data = m_tabBar->tabData(index);
    //qDebug() << "current tab data: " << data.toInt();
    m_proxy->setFilterTrack(data.toInt()-1);
    m_model->setCurrentTrack(data.toInt()-1);
}

bool KMidimon::askTrackFilter(int& track)
{
    bool result;
    track = KInputDialog::getInteger( i18n("Change track"),
                i18n("Change the track filter:"),
                track, 1, 255, 1, 10, &result, this );
    return result;
}

void KMidimon::addTrack()
{
    int track = 1;
    if (askTrackFilter(track)) {
        addNewTab(track);
    }
}

void KMidimon::deleteTrack(int tabIndex)
{
    if (m_tabBar->count() > 1) {
        m_tabBar->removeTab(tabIndex);
    }
}

void KMidimon::changeTrack(int tabIndex)
{
    int track = m_tabBar->tabData(tabIndex).toInt();
    if (askTrackFilter(track)) {
        //qDebug() << "changeTrack: " << tabIndex;
        QString tabName = i18n("Track %1", track);
        m_tabBar->setTabData(tabIndex, track);
        m_tabBar->setTabText(tabIndex, tabName);
        m_tabBar->setTabWhatsThis(tabIndex, i18n("Track %1 View Selector", track));
        tabIndexChanged(tabIndex);
    }
}

void KMidimon::deleteCurrentTrack()
{
    deleteTrack(m_tabBar->currentIndex());
}

void KMidimon::changeCurrentTrack()
{
    changeTrack(m_tabBar->currentIndex());
}

void KMidimon::reorderTabs(int fromIndex, int toIndex)
{
    //qDebug() << "reorderTabs(" << fromIndex << "," << toIndex << ")";
    QIcon icon = m_tabBar->tabIcon(fromIndex);
    QString text = m_tabBar->tabText(fromIndex);
    QVariant data = m_tabBar->tabData(fromIndex);
    m_tabBar->removeTab(fromIndex);
    m_tabBar->insertTab(toIndex, icon, text);
    m_tabBar->setTabData(toIndex, data);
    m_tabBar->setCurrentIndex(toIndex);
}

void KMidimon::slotTicks(int row)
{
    QModelIndex index = m_model->getRowIndex(row);
    if (index.isValid()) {
        QModelIndex vidx = m_proxy->mapFromSource(index);
        if (vidx.isValid())
            m_view->setCurrentIndex(vidx);
    }
}

void
KMidimon::slotCurrentChanged( const QModelIndex& curr,
                              const QModelIndex& /*prev*/ )
{
    if (m_adaptor->isPlaying() | m_adaptor->isRecording())
        return;
    QModelIndex idx = m_proxy->mapToSource(curr);
    m_adaptor->setPosition(idx.row());
}

void
KMidimon::updateView()
{
    QModelIndex index = m_model->getCurrentRow();
    if (index.isValid())
        m_view->setCurrentIndex(m_proxy->mapFromSource(index));
}
