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

#include "kmidimon.h"
#include "configdialog.h"
#include "connectdlg.h"
#include "sequencemodel.h"
#include "proxymodel.h"
#include "eventfilter.h"
#include "sequenceradaptor.h"
#include "slideraction.h"
#include "ui_kmidimonwin.h"
#include "about.h"

#include <QMenu>
#include <QEvent>
#include <QContextMenuEvent>
#include <QCursor>
#include <QTreeView>
#include <QTextStream>
#include <QVariant>
#include <QToolTip>
#include <QFileInfo>
#include <QDropEvent>
#include <QMessageBox>
#include <QMenuBar>
#include <QMimeData>
#include <QDateTime>
#include <QProgressDialog>
#include <QFileDialog>
#include <QSettings>
#include <QInputDialog>
#include <QFontDatabase>
#include <QFont>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QDesktopServices>

QDir KMidimon::dataDirectory()
{
    QDir test(QApplication::applicationDirPath() + "/../share/kmidimon/");
    if (test.exists())
        return test;
    QStringList candidates = QStandardPaths::standardLocations(QStandardPaths::DataLocation);
    foreach(const QString& d, candidates) {
        test = QDir(d);
        if (test.exists())
            return test;
    }
    return QDir();
}

QDir KMidimon::localeDirectory()
{
    QDir data = dataDirectory();
    data.cd("locale");
    return data;
}

KMidimon::KMidimon() :
    QMainWindow(0),
    m_state(InvalidState),
    m_adaptor(0)
{
    m_trq = new QTranslator(this);
    m_trp = new QTranslator(this);
    m_trq->load( "qt_" + configuredLanguage(), QLibraryInfo::location(QLibraryInfo::TranslationsPath) );
    m_trp->load( configuredLanguage(), localeDirectory().absolutePath() );
    QApplication::installTranslator(m_trq);
    QApplication::installTranslator(m_trp);

    m_ui = new Ui::KMidimonWin();
    m_ui->setupUi(this);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    m_useFixedFont = false;
    m_autoResizeColumns = false;
    m_requestRealtimePrio = false;
    m_model = new SequenceModel(this);
    m_proxy = new ProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_view = new QTreeView(this);
    m_view->setWhatsThis(tr("The events list"));
    m_view->setRootIsDecorated(false);
    m_view->setAlternatingRowColors(true);
    m_view->setModel(m_proxy);
    m_view->setSortingEnabled(false);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    setAcceptDrops(true);
    connect( m_view->selectionModel(),
             SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
             SLOT(slotCurrentChanged(const QModelIndex&, const QModelIndex&)) );
    try {
        m_adaptor = new SequencerAdaptor(this);
        m_adaptor->setModel(m_model);
        m_adaptor->updateModelClients();
        connect( m_model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                          SLOT(modelRowsInserted(QModelIndex,int,int)) );
        connect( m_adaptor, SIGNAL(signalTicks(int)), SLOT(slotTicks(int)));
        connect( m_adaptor, SIGNAL(finished()), SLOT(songFinished()));
        m_tabBar = new QTabBar(this);
        m_tabBar->setWhatsThis(tr("Track view selectors"));
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
//        connect( m_tabBar, SIGNAL(newTabRequest()),
//                           SLOT(addTrack()) );
        connect( m_tabBar, &QTabBar::tabBarDoubleClicked, this, &KMidimon::changeTrack );
        connect( m_tabBar, SIGNAL(currentChanged(int)),
                           SLOT(tabIndexChanged(int)) );
        layout->addWidget(m_tabBar);
        layout->addWidget(m_view);
        vbox->setLayout(layout);
        setCentralWidget(vbox);
        setupActions();
        createLanguageMenu();
        readConfiguration();
        fileNew();
        record();
    } catch (SequencerError& ex) {
        QString errorstr = tr("Fatal error from the ALSA sequencer. "
            "This usually happens when the kernel doesn't have ALSA support, "
            "or the device node (/dev/snd/seq) doesn't exists, "
            "or the kernel module (snd_seq) is not loaded. "
            "Please check your ALSA/MIDI configuration. Returned error was: %1")
                .arg(ex.qstrError());
        QMessageBox::critical(0, tr("Error"), errorstr);
        close();
    }
}

void KMidimon::setupActions()
{
    const QString columnName[COLUMN_COUNT] = {
            tr("Ticks"),
            tr("Time"),
            tr("Source", "event origin"),
            tr("Event Kind", "type of event"),
            tr("Channel"),
            tr("Data 1"),
            tr("Data 2"),
            tr("Data 3")
    };
    /*const QString actionName[COLUMN_COUNT] = {
            "show_ticks",
            "show_time",
            "show_source",
            "show_kind",
            "show_channel",
            "show_data1",
            "show_data2",
            "show_data3"
    };*/

    QAction* a = new QAction(this);
    a->setText(tr("&New"));
    a->setIcon(QIcon::fromTheme("document-new"));
    a->setWhatsThis(tr("Clear the current data and start a new empty session"));
    connect(a, SIGNAL(triggered()), SLOT(fileNew()));
    m_ui->menuFile->addAction(a);
    m_ui->toolBar->addAction(a);

    a = new QAction(this);
    a->setText(tr("&Open"));
    a->setIcon(QIcon::fromTheme("document-open"));
    a->setWhatsThis(tr("Open a disk file"));
    connect(a, SIGNAL(triggered()), SLOT(fileOpen()));
    m_ui->menuFile->addAction(a);
    m_ui->toolBar->addAction(a);

    m_recentFiles = m_ui->menuFile->addMenu(QIcon::fromTheme("document-open-recent"), "Recent files");
    connect(m_recentFiles, &QMenu::aboutToShow, this, &KMidimon::updateRecentFileActions);
    m_recentFileSubMenuAct = m_recentFiles->menuAction();

    for (int i = 0; i < MaxRecentFiles; ++i) {
        m_recentFileActs[i] = m_recentFiles->addAction(QString(), this, &KMidimon::openRecentFile);
        m_recentFileActs[i]->setVisible(false);
    }

    m_recentFileSeparator = m_ui->menuFile->addSeparator();
    setRecentFilesVisible(KMidimon::hasRecentFiles());

    m_save = new QAction(this);
    m_save->setText(tr("&Save"));
    m_save->setIcon(QIcon::fromTheme("document-save"));
    m_save->setWhatsThis(tr("Store the session data on a disk file"));
    connect(m_save, SIGNAL(triggered()), SLOT(fileSave()));
    m_ui->menuFile->addAction(m_save);
    m_ui->toolBar->addAction(m_save);

    m_fileInfo = new QAction(this);
    m_fileInfo->setText(tr("Sequence Info"));
    m_fileInfo->setWhatsThis(tr("Display information about the loaded sequence"));
    m_fileInfo->setIcon(QIcon::fromTheme("dialog-information"));
    connect(m_fileInfo, SIGNAL(triggered()), SLOT(songFileInfo()));
    m_ui->menuFile->addAction(m_fileInfo);
    m_ui->menuFile->addSeparator();

    a = new QAction(this);
    a->setText(tr("Quit"));
    a->setIcon(QIcon::fromTheme("application-exit"));
    a->setWhatsThis(tr("Exit the application"));
    connect(a, SIGNAL(triggered()), SLOT(close()));
    m_ui->menuFile->addAction(a);
    m_ui->toolBar->addAction(a);

    m_prefs = new QAction(this);
    m_prefs->setText(tr("Preferences"));
    m_prefs->setIcon(QIcon::fromTheme("configure"));
    m_prefs->setWhatsThis(tr("Configure the program setting several preferences"));
    connect(m_prefs, SIGNAL(triggered()), SLOT(preferences()));
    m_ui->menuSettings->addAction(m_prefs);
    m_ui->toolBar->addAction(m_prefs);

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

    m_rewind = new QAction(this);
    m_rewind->setText(tr("Backward","player skip backward"));
    m_rewind->setIcon(QIcon::fromTheme("media-skip-backward"));
    m_rewind->setWhatsThis(tr("Move the playback position to the first event"));
    connect(m_rewind, SIGNAL(triggered()), SLOT(rewind()));
    m_ui->menuControl->addAction(m_rewind);
    m_ui->toolBar->addAction(m_rewind);

    m_play = new QAction(this);
    m_play->setText(tr("&Play"));
    m_play->setIcon(QIcon::fromTheme("media-playback-start"));
    m_play->setShortcut( Qt::Key_MediaPlay );
    m_play->setWhatsThis(tr("Start playback of the current session"));
    connect(m_play, SIGNAL(triggered()), SLOT(play()));
    m_ui->menuControl->addAction(m_play);
    m_ui->toolBar->addAction(m_play);

    m_pause = new QAction(this);
    m_pause->setText(tr("Pause"));
    m_pause->setCheckable(true);
    m_pause->setIcon(QIcon::fromTheme("media-playback-pause"));
    m_pause->setWhatsThis(tr("Pause the playback"));
    connect(m_pause, SIGNAL(triggered()), SLOT(pause()));
    m_ui->menuControl->addAction(m_pause);
    m_ui->toolBar->addAction(m_pause);

    m_forward = new QAction(this);
    m_forward->setText(tr("Forward","player skip forward"));
    m_forward->setIcon(QIcon::fromTheme("media-skip-forward"));
    m_forward->setWhatsThis(tr("Move the playback position to the last event"));
    connect(m_forward, SIGNAL(triggered()), SLOT(forward()));
    m_ui->menuControl->addAction(m_forward);
    m_ui->toolBar->addAction(m_forward);

    m_record = new QAction(this);
    m_record->setText(tr("Record"));
    m_record->setIcon(QIcon::fromTheme("media-record"));
    m_record->setShortcut( Qt::Key_MediaRecord );
    m_record->setWhatsThis(tr("Append new recorded events to the current session"));
    connect(m_record, SIGNAL(triggered()), SLOT(record()));
    m_ui->menuControl->addAction(m_record);
    m_ui->toolBar->addAction(m_record);

    m_stop = new QAction(this);
    m_stop->setText( tr("Stop") );
    m_stop->setIcon(QIcon::fromTheme("media-playback-stop"));
    m_stop->setShortcut( Qt::Key_MediaStop );
    m_stop->setWhatsThis(tr("Stop playback or recording"));
    connect(m_stop, SIGNAL(triggered()), SLOT(stop()));
    m_ui->menuControl->addAction(m_stop);
    m_ui->toolBar->addAction(m_stop);

    m_ui->menuControl->addSeparator();

    m_loop = new QAction(this);
    m_loop->setCheckable(true);
    m_loop->setText(tr("Player Loop"));
    m_loop->setWhatsThis(tr("Start playing again at song ending"));
    connect(m_loop, SIGNAL(triggered()), SLOT(slotLoop()));
    m_ui->menuControl->addAction(m_loop);

    m_tempoSlider = new PlayerPopupSliderAction( this, SLOT(tempoSlider(int)), this );
    m_tempoSlider->setText(tr("Scale Tempo"));
    m_tempoSlider->setWhatsThis(tr("Display a slider to scale the tempo between 50% and 200%"));
    m_tempoSlider->setIcon(QIcon::fromTheme("chronometer"));
    m_ui->menuControl->addAction(m_tempoSlider);
    m_ui->toolBar->addAction(m_tempoSlider);

    m_tempo100 = new QAction(this);
    m_tempo100->setText(tr("Reset Tempo"));
    m_tempo100->setWhatsThis(tr("Reset the tempo scale to 100%"));
    m_tempo100->setIcon(QIcon::fromTheme("player-time"));
    connect(m_tempo100, SIGNAL(triggered()), this, SLOT(tempoReset()));
    m_ui->menuControl->addAction(m_tempo100);
    m_ui->toolBar->addAction(m_tempo100);

    m_connectAll = new QAction(this);
    m_connectAll->setText(tr("Connect All Inputs"));
    m_connectAll->setWhatsThis(tr("Connect all readable MIDI ports"));
    connect(m_connectAll, SIGNAL(triggered()), SLOT(connectAll()));
    m_ui->menuConnections->addAction(m_connectAll);

    m_disconnectAll = new QAction(this);
    m_disconnectAll->setText(tr("Disconnect All Inputs"));
    m_disconnectAll->setWhatsThis(tr("Disconnect all input MIDI ports"));
    connect(m_disconnectAll, SIGNAL(triggered()), SLOT(disconnectAll()));
    m_ui->menuConnections->addAction(m_disconnectAll);

    m_ui->menuConnections->addSeparator();

    m_configConns = new QAction(this);
    m_configConns->setText(tr("Configure Connections"));
    m_configConns->setWhatsThis(tr("Open the Connections dialog"));
    connect(m_configConns, SIGNAL(triggered()), SLOT(configConnections()));
    m_ui->menuConnections->addAction(m_configConns);

    m_popup = new QMenu(this);

    m_resizeColumns = new QAction(this);
    m_resizeColumns->setText(tr("Resize columns"));
    m_resizeColumns->setWhatsThis(tr("Resize the columns width to fit it's contents"));
    connect(m_resizeColumns, SIGNAL(triggered()), SLOT(resizeAllColumns()));
    m_popup->addAction(m_resizeColumns);

    QMenu* tracks = m_popup->addMenu(tr("Tracks"));

    m_createTrack = new QAction(this);
    m_createTrack->setText(tr("Add Track View"));
    m_createTrack->setWhatsThis(tr("Create a new tab/track view"));
    connect(m_createTrack, SIGNAL(triggered()), SLOT(addTrack()));
    tracks->addAction(m_createTrack);

    m_deleteTrack = new QAction(this);
    m_deleteTrack->setText(tr("Delete Track View"));
    m_deleteTrack->setWhatsThis(tr("Delete the tab/track view"));
    connect(m_deleteTrack, SIGNAL(triggered()), SLOT(deleteCurrentTrack()));
    tracks->addAction(m_deleteTrack);

    m_changeTrack = new QAction(this);
    m_changeTrack->setText(tr("Change Track View"));
    m_changeTrack->setWhatsThis(tr("Change the track number of the view"));
    connect(m_changeTrack, SIGNAL(triggered()), SLOT(changeCurrentTrack()));
    tracks->addAction(m_changeTrack);

    m_muteTrack = new QAction(this);
    m_muteTrack->setCheckable(true);
    m_muteTrack->setText(tr("Mute Track"));
    m_muteTrack->setWhatsThis(tr("Mute (silence) the track"));
    connect(m_muteTrack, SIGNAL(triggered()), SLOT(muteCurrentTrack()));
    tracks->addAction(m_muteTrack);

    QMenu* columns = m_popup->addMenu(tr("Show Columns"));

    for ( int i = 0; i < COLUMN_COUNT; ++i ) {
        m_popupAction[i] = new QAction(columnName[i], this);
        m_popupAction[i]->setCheckable(true);
        m_popupAction[i]->setChecked(true);
        m_popupAction[i]->setWhatsThis(tr("Toggle the %1 column").arg(columnName[i]));
        connect(m_popupAction[i], &QAction::triggered, [=] {toggleColumn(i);});
        columns->addAction(m_popupAction[i]);
    }

    m_filter = new EventFilter(this);
    QMenu* filtersMenu = m_filter->buildMenu(this);
    m_popup->addMenu( filtersMenu );
    m_model->setFilter(m_filter);
    m_proxy->setFilter(m_filter);
    connect(m_filter, SIGNAL(filterChanged()), m_proxy, SLOT(invalidate()));
    menuBar()->insertMenu( menuBar()->actions().last(), filtersMenu );

    m_ui->actionAbout_Qt->setIcon(QIcon(":/qt-project.org/qmessagebox/images/qtlogo-64.png"));
    connect( m_ui->actionAbout, SIGNAL(triggered()), SLOT(about()) );
    connect( m_ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()) );
    connect( m_ui->actionContents, SIGNAL(triggered()), SLOT(help()) );
    connect( m_ui->actionWeb_Site, SIGNAL(triggered()), SLOT(slotOpenWebSite()) );
}

void KMidimon::about()
{
    About dlg(this);
    dlg.exec();
}

void KMidimon::help()
{
    QString hlpFile = QStringLiteral("kmidimon.html");
    QDir data = dataDirectory();
    QFileInfo finfo(data.filePath(hlpFile));
    if (finfo.exists()) {
        QUrl url = QUrl::fromLocalFile(finfo.absoluteFilePath());
        QDesktopServices::openUrl(url);
    }
}

void KMidimon::slotOpenWebSite()
{
    QUrl url(QStringLiteral("http://kmidimon.sourceforge.net"));
    QDesktopServices::openUrl(url);
}

void KMidimon::fileNew()
{
    m_currentFile.clear();
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
    tempoReset();
    updateView();
    updateCaption();
}

void KMidimon::open(const QString& fileName)
{
    try {
        QFileInfo finfo(fileName);
        m_currentFile = finfo.absoluteFilePath();
        m_view->blockSignals(true);
        stop();
        m_model->clear();
        for (int i = m_tabBar->count() - 1; i >= 0; i--) {
            m_tabBar->removeTab(i);
        }
        m_pd = new QProgressDialog(this);
        m_pd->setLabelText(tr("Loading..."));
        m_pd->setWindowTitle(tr("Load file"));
        m_pd->setCancelButton(0);
        m_pd->setMinimumDuration(500);
        m_pd->setRange(0, finfo.size());
        m_pd->setValue(0);
        connect( m_model, SIGNAL(loadProgress(int)), m_pd, SLOT(setValue(int)) );
        m_model->loadFromFile(fileName);
        m_pd->setValue(finfo.size());
        m_adaptor->setResolution(m_model->getSMFDivision());
        if (m_model->getInitialTempo() > 0) {
            m_adaptor->setTempo(m_model->getInitialTempo());
            m_adaptor->queue_set_tempo();
        }
        m_adaptor->rewind();
        int ntrks = m_model->getSMFTracks();
        if (ntrks < 1) ntrks = 1;
        for (int i = 0; i < ntrks; i++)
            addNewTab(m_model->getTrackForIndex(i));
        m_tabBar->setCurrentIndex(0);
        m_proxy->setFilterTrack(0);
        m_model->setCurrentTrack(0);
        for (int i = 0; i < COLUMN_COUNT; ++i)
            m_view->resizeColumnToContents(i);
        tempoReset();
    } catch (...) {
        m_model->clear();
    }
    m_view->blockSignals(false);
    prependToRecentFiles(fileName);
    updateCaption();
    delete m_pd;
    QString loadingMsg = m_model->getLoadingMessages();
    if (!loadingMsg.isEmpty()) {
        loadingMsg.insert(0, tr("Warning, this file may be non-standard or damaged.<br/>"));
        QMessageBox::warning(this, tr("File parsing error"), loadingMsg);
    }
}

void KMidimon::fileOpen()
{
    QFileDialog fd(this, tr("Open MIDI file"));
    QStringList filters;
    filters << "audio/midi" << "audio/cakewalk" << "audio/overture";
    fd.setMimeTypeFilters(filters);
    fd.selectMimeTypeFilter(filters[0]);
    fd.setFileMode(QFileDialog::ExistingFile);
    fd.setAcceptMode(QFileDialog::AcceptOpen);
    if (fd.exec()) {
        QStringList fs = fd.selectedFiles();
        if (fs.count() >= 1) open(fs[0]);
    }
}

void KMidimon::fileSave()
{
    QFileDialog fd(this, tr("Save MIDI monitor data"));
    QString filters = tr("Plain text files (*.txt);;MIDI files (*.mid)");
    fd.setNameFilter(filters);
    fd.setFileMode(QFileDialog::AnyFile);
    fd.setAcceptMode(QFileDialog::AcceptSave);
    if (fd.exec()) {
        QString path = fd.selectedFiles().first();
        if (!path.isNull()) {
            m_model->saveToFile(path);
            prependToRecentFiles(path);
        }
    }
}

KMidimon::~KMidimon()
{
    delete m_ui;
}

void KMidimon::closeEvent(QCloseEvent *event)
{
    stop();
    saveConfiguration();
    event->accept();
}

void KMidimon::saveConfiguration()
{
    int i;
    QSettings config;
    config.beginGroup("Settings");
    if (m_adaptor == NULL) return;
    config.setValue("resolution", m_defaultResolution);
    config.setValue("tempo", m_defaultTempo);
    config.setValue("realtime_prio", m_requestRealtimePrio);
    config.setValue("alsa", m_proxy->showAlsaMsg());
    config.setValue("channel", m_proxy->showChannelMsg());
    config.setValue("common", m_proxy->showCommonMsg());
    config.setValue("realtime", m_proxy->showRealTimeMsg());
    config.setValue("sysex", m_proxy->showSysexMsg());
    config.setValue("smf", m_proxy->showSmfMsg());
    config.setValue("client_names", m_model->showClientNames());
    config.setValue("translate_sysex", m_model->translateSysex());
    config.setValue("translate_notes", m_model->translateNotes());
    config.setValue("translate_ctrls", m_model->translateCtrls());
    config.setValue("instrument", m_model->getInstrumentName());
    config.setValue("encoding", m_model->getEncoding());
    config.setValue("fixed_font", getFixedFont());
    config.setValue("auto_resize", m_autoResizeColumns);
    for (i = 0; i < COLUMN_COUNT; ++i) {
        config.setValue(QString("show_column_%1").arg(i),
                m_popupAction[i]->isChecked());
    }
    config.setValue("output_connection", m_outputConn);
    config.sync();
    m_filter->saveConfiguration();
}

void KMidimon::readConfiguration()
{
    int i;
    bool status;
    QSettings config;
    config.beginGroup("Settings");
    m_proxy->setFilterAlsaMsg(config.value("alsa", true).toBool());
    m_proxy->setFilterChannelMsg(config.value("channel", true).toBool());
    m_proxy->setFilterCommonMsg(config.value("common", true).toBool());
    m_proxy->setFilterRealTimeMsg(config.value("realtime", true).toBool());
    m_proxy->setFilterSysexMsg(config.value("sysex", true).toBool());
    m_proxy->setFilterSmfMsg(config.value("smf", true).toBool());
    m_model->setShowClientNames(config.value("client_names", false).toBool());
    m_model->setTranslateSysex(config.value("translate_sysex", false).toBool());
    m_model->setTranslateNotes(config.value("translate_notes", false).toBool());
    m_model->setTranslateCtrls(config.value("translate_ctrls", false).toBool());
    m_model->setInstrumentName(config.value("instrument", QString()).toString());
    m_model->setEncoding(config.value("encoding", QString()).toString());
    m_autoResizeColumns = config.value("auto_resize", false).toBool();
    m_defaultResolution = config.value("resolution", RESOLUTION).toInt();
    m_defaultTempo = config.value("tempo", TEMPO_BPM).toInt();
    m_requestRealtimePrio = config.value("realtime_prio", false).toBool();
    m_adaptor->setResolution(m_defaultResolution);
    m_adaptor->setTempo(m_defaultTempo);
    m_adaptor->queue_set_tempo();
    m_adaptor->setRequestRealtime(m_requestRealtimePrio);
    setFixedFont(config.value("fixed_font", false).toBool());
    for (i = 0; i < COLUMN_COUNT; ++i) {
        status = config.value(QString("show_column_%1").arg(i), true).toBool();
        setColumnStatus(i, status);
    }
    m_outputConn = config.value("output_connection", QString()).toString();
    m_adaptor->connect_output(m_outputConn);
    m_filter->loadConfiguration();
}

void KMidimon::preferences()
{
    int i;
    bool was_running;
    QPointer<ConfigDialog> dlg = new ConfigDialog(this);
    dlg->setTempo(m_defaultTempo);
    dlg->setResolution(m_defaultResolution);
    dlg->setRequestRealtime(m_requestRealtimePrio);
    dlg->setRegAlsaMsg(m_proxy->showAlsaMsg());
    dlg->setRegChannelMsg(m_proxy->showChannelMsg());
    dlg->setRegCommonMsg(m_proxy->showCommonMsg());
    dlg->setRegRealTimeMsg(m_proxy->showRealTimeMsg());
    dlg->setRegSysexMsg(m_proxy->showSysexMsg());
    dlg->setRegSmfMsg(m_proxy->showSmfMsg());
    dlg->setShowClientNames(m_model->showClientNames());
    dlg->setTranslateSysex(m_model->translateSysex());
    dlg->setTranslateNotes(m_model->translateNotes());
    dlg->setTranslateCtrls(m_model->translateCtrls());
    dlg->setInstruments(m_model->getInstruments());
    dlg->setInstrumentName(m_model->getInstrumentName());
    dlg->setEncoding(m_model->getEncoding());
    dlg->setUseFixedFont(getFixedFont());
    dlg->setResizeColumns(m_autoResizeColumns);
    for (i = 0; i < COLUMN_COUNT; ++i) {
        dlg->setShowColumn(i, m_popupAction[i]->isChecked());
    }
    if (dlg->exec() == QDialog::Accepted) {
        if (dlg != NULL) {
            was_running = m_adaptor->isRecording();
            if (was_running) stop();
            m_proxy->setFilterAlsaMsg(dlg->isRegAlsaMsg());
            m_proxy->setFilterChannelMsg(dlg->isRegChannelMsg());
            m_proxy->setFilterCommonMsg(dlg->isRegCommonMsg());
            m_proxy->setFilterRealTimeMsg(dlg->isRegRealTimeMsg());
            m_proxy->setFilterSysexMsg(dlg->isRegSysexMsg());
            m_proxy->setFilterSmfMsg(dlg->isRegSmfMsg());
            m_model->setShowClientNames(dlg->showClientNames());
            m_model->setTranslateSysex(dlg->translateSysex());
            m_model->setTranslateNotes(dlg->translateNotes());
            m_model->setTranslateCtrls(dlg->translateCtrls());
            m_model->setInstrumentName(dlg->getInstrumentName());
            m_model->setEncoding(dlg->getEncoding());
            m_autoResizeColumns = dlg->resizeColumns();
            m_defaultTempo = dlg->getTempo();
            m_defaultResolution = dlg->getResolution();
            setFixedFont(dlg->useFixedFont());
            for (i = 0; i < COLUMN_COUNT; ++i) {
                setColumnStatus(i, dlg->showColumn(i));
            }
            m_requestRealtimePrio = dlg->requestRealtime();
            m_adaptor->setRequestRealtime(m_requestRealtimePrio);
            if (was_running) record();
        }
    }
    delete dlg;
}

void KMidimon::record()
{
    m_adaptor->forward();
    m_adaptor->record();
    if (m_adaptor->isRecording())
        updateState(RecordingState);
}

void KMidimon::stop()
{
    if ( m_adaptor->isRecording() ||
         m_adaptor->isPlaying() ||
         m_adaptor->isPaused()) {
        m_adaptor->stop();
        songFinished();
    }
}

void KMidimon::songFinished()
{
    updateState(StoppedState);
    updateView();
}

void KMidimon::play()
{
    m_adaptor->play();
    if (m_adaptor->isPlaying()) {
        updateState(PlayingState);
    }
}

void KMidimon::pause()
{
    m_adaptor->pause(m_pause->isChecked());
    if (m_adaptor->isPaused())
        updateState(PausedState);
    else
        updateState(PlayingState);
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

void KMidimon::updateCaption()
{
    QFileInfo finfo(m_currentFile);
    QString name = finfo.fileName();
    if (name.isEmpty())
        name = tr("(no file)");
    setWindowTitle(tr("%1 [%2]").arg(name).arg(m_currentState));
}

void KMidimon::updateState(PlayerState newState)
{
    if (m_state == newState)
        return;
    switch (newState) {
    case InvalidState:
        m_rewind->setEnabled(false);
        m_play->setEnabled(false);
        m_pause->setEnabled(false);
        m_forward->setEnabled(false);
        m_record->setEnabled(false);
        m_stop->setEnabled(false);
        m_tempoSlider->setEnabled(false);
        m_tempo100->setEnabled(false);
        statusBar()->showMessage(m_currentState = tr("empty"));
        break;
    case RecordingState:
        m_rewind->setEnabled(false);
        m_play->setEnabled(false);
        m_pause->setEnabled(false);
        m_forward->setEnabled(false);
        m_record->setEnabled(false);
        m_stop->setEnabled(true);
        m_tempoSlider->setEnabled(false);
        m_tempo100->setEnabled(false);
        statusBar()->showMessage(m_currentState = tr("recording"));
        break;
    case PlayingState:
        m_rewind->setEnabled(false);
        m_play->setEnabled(false);
        m_pause->setEnabled(true);
        m_forward->setEnabled(false);
        m_record->setEnabled(false);
        m_stop->setEnabled(true);
        m_tempoSlider->setEnabled(true);
        m_tempo100->setEnabled(true);
        statusBar()->showMessage(m_currentState = tr("playing"));
        break;
    case PausedState:
        m_rewind->setEnabled(false);
        m_play->setEnabled(false);
        m_pause->setEnabled(true);
        m_forward->setEnabled(false);
        m_record->setEnabled(false);
        m_stop->setEnabled(true);
        m_tempoSlider->setEnabled(false);
        m_tempo100->setEnabled(false);
        statusBar()->showMessage(m_currentState = tr("paused"));
        break;
    case StoppedState:
        m_rewind->setEnabled(true);
        m_play->setEnabled(true);
        m_pause->setEnabled(false);
        m_forward->setEnabled(true);
        m_record->setEnabled(true);
        m_stop->setEnabled(false);
        m_tempoSlider->setEnabled(true);
        m_tempo100->setEnabled(true);
        statusBar()->showMessage(m_currentState = tr("stopped"));
        break;
    default:
        statusBar()->showMessage(m_currentState = tr("uninitialized"));
        break;
    }
    updateCaption();
    m_pause->setChecked(m_adaptor->isPaused());
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
    QPointer<ConnectDlg> dlg = new ConnectDlg(this, inputs, subs, outputs, m_outputConn);
    if (dlg->exec() == QDialog::Accepted) {
        if (dlg != NULL) {
            QStringList desired = dlg->getSelectedInputs();
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
            QString newOut = dlg->getSelectedOutput();
            if (newOut != m_outputConn) {
                m_adaptor->disconnect_output(m_outputConn);
                m_adaptor->connect_output(m_outputConn = newOut);
            }
        }
    }
    delete dlg;
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
    QVariant data = m_tabBar->tabData(m_tabBar->currentIndex());
    int track = data.toInt() - 1;
    m_muteTrack->setChecked(m_model->getSong()->mutedState(track));
    m_popup->exec(QCursor::pos());
}

void KMidimon::setFixedFont(bool newValue)
{
    if (m_useFixedFont != newValue) {
        m_useFixedFont = newValue;
        m_view->setFont(m_useFixedFont ?
                        QFontDatabase::systemFont(QFontDatabase::FixedFont) :
                        QFontDatabase::systemFont(QFontDatabase::GeneralFont));
    }
}

void KMidimon::modelRowsInserted(const QModelIndex&, int, int)
{
    if (m_autoResizeColumns)
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
    QString tabName = tr("Track %1","song track").arg(data);
    int i = m_tabBar->addTab(tabName);
    m_tabBar->setTabData(i, QVariant(data));
    m_tabBar->setTabWhatsThis(i, tr("Track %1 View Selector","track selector").arg(data));
}

void KMidimon::tabIndexChanged(int index)
{
    QVariant data = m_tabBar->tabData(index);
    m_proxy->setFilterTrack(data.toInt()-1);
    m_model->setCurrentTrack(data.toInt()-1);
}

bool KMidimon::askTrackFilter(int& track)
{
    bool result;
    track = QInputDialog::getInt(this, tr("Change track"),
                  tr("Change the track filter:"),
                  track, 1, 255, 1, &result);
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
        QString tabName = tr("Track %1","song track").arg(track);
        m_tabBar->setTabData(tabIndex, track);
        m_tabBar->setTabText(tabIndex, tabName);
        m_tabBar->setTabWhatsThis(tabIndex, tr("Track %1 View Selector","track selector").arg(track));
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

void
KMidimon::songFileInfo()
{
    QString infostr;
    if (m_currentFile.isEmpty())
        infostr = tr("No file loaded");
    else {
        QFileInfo finfo(m_currentFile);
        infostr = tr("File: <b>%1</b><br/>"
                       "Created: <b>%2</b><br/>"
                       "Modified: <b>%3</b><br/>"
                       "Format: <b>%4</b><br/>"
                       "Number of tracks: <b>%5</b><br/>"
                       "Number of events: <b>%6</b><br/>"
                       "Division: <b>%7 ppq</b><br/>"
                       "Initial tempo: <b>%8 bpm</b><br/>"
                       "Duration: <b>%9</b>")
                       .arg(finfo.fileName())
                       .arg(finfo.created().toString(Qt::DefaultLocaleLongDate))
                       .arg(finfo.lastModified().toString(Qt::DefaultLocaleLongDate))
                       .arg(m_model->getFileFormat())
                       .arg(m_model->getSMFTracks())
                       .arg(m_model->getSong()->size())
                       .arg(m_model->getSMFDivision())
                       .arg(m_model->getInitialTempo())
                       .arg(m_model->getDuration());
    }
    QMessageBox::information(this, tr("Sequence Information"), infostr);
}

void KMidimon::tempoReset()
{
    m_adaptor->setTempoFactor(1.0);
    m_tempoSlider->slider()->setValue(100);
    m_tempoSlider->slider()->setToolTip("100%");
}

void KMidimon::tempoSlider(int value)
{
    double tempoFactor = (value*value + 100.0*value + 2e4) / 4e4;
    m_adaptor->setTempoFactor(tempoFactor);
    // Slider tooltip
    QString tip = QString("%1\%").arg(tempoFactor*100.0, 0, 'f', 0);
    m_tempoSlider->slider()->setToolTip(tip);
    QToolTip::showText(QCursor::pos(), tip, this);
}

void KMidimon::muteTrack(int tabIndex)
{
    QVariant data = m_tabBar->tabData(tabIndex);
    int track = data.toInt() - 1;
    Song* song = m_model->getSong();
    if (song != NULL) {
        bool newState = !song->mutedState(track);
        song->setMutedState(track, newState);
        if (newState) m_adaptor->removeTrackEvents(track);
    }
}

void KMidimon::muteCurrentTrack()
{
    muteTrack(m_tabBar->currentIndex());
}

void KMidimon::slotLoop()
{
    m_adaptor->setLoop(m_loop->isChecked());
}

void KMidimon::dragEnterEvent( QDragEnterEvent * event )
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void KMidimon::dropEvent( QDropEvent * event )
{
    if ( event->mimeData()->hasUrls() ) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.empty())
            open(urls.first().fileName());
        event->accept();
    }
}

QString KMidimon::configuredLanguage()
{
    if (m_language.isEmpty()) {
        QSettings settings;
        QString defLang = QLocale::system().name();
        settings.beginGroup("Settings");
        m_language = settings.value("language", defLang).toString();
        settings.endGroup();
    }
    //qDebug() << Q_FUNC_INFO << m_language;
    return m_language;
}

void KMidimon::slotSwitchLanguage(QAction *action)
{
    QString lang = action->data().toString();
    QLocale qlocale(lang);
    QString localeName = qlocale.nativeLanguageName();
    if ( QMessageBox::question (this, tr("Language Changed"),
            tr("The language for this application is going to change to %1. "
               "Do you want to continue?").arg(localeName),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes )
    {
        m_language = lang;
        retranslateUi();
    } else {
        m_currentLang->setChecked(true);
    }
}

void KMidimon::createLanguageMenu()
{
    QString currentLang = configuredLanguage();
    QActionGroup *languageGroup = new QActionGroup(this);
    connect(languageGroup, SIGNAL(triggered(QAction *)),
            SLOT(slotSwitchLanguage(QAction *)));
    QDir dir = localeDirectory();
    QStringList fileNames = dir.entryList(QStringList("*.qm"));
    QStringList locales;
    locales << "en";
    foreach (const QString& fileName, fileNames) {
        QString locale = fileName;
        locale.truncate(locale.lastIndexOf('.'));
        locales << locale;
    }
    locales.sort();
    m_ui->menuLanguage->clear();
    foreach (const QString& loc, locales) {
        QLocale qlocale(loc);
        QString localeName = QLocale::languageToString(qlocale.language()); //qlocale.nativeLanguageName();
        QAction *action = new QAction(localeName, this);
        action->setCheckable(true);
        action->setData(loc);
        m_ui->menuLanguage->addAction(action);
        languageGroup->addAction(action);
        if (currentLang.startsWith(loc)) {
            action->setChecked(true);
            m_currentLang = action;
        }
    }
}

void KMidimon::retranslateUi()
{
    auto lang = configuredLanguage();
    m_trq->load( "qt_" + lang, QLibraryInfo::location(QLibraryInfo::TranslationsPath) );
    m_trp->load( lang, localeDirectory().absolutePath() );
    m_ui->retranslateUi(this);
    createLanguageMenu();
}

void KMidimon::setRecentFilesVisible(bool visible)
{
    m_recentFileSubMenuAct->setVisible(visible);
    m_recentFileSeparator->setVisible(visible);
}

QStringList KMidimon::readRecentFiles(QSettings &settings)
{
    QStringList result;
    const int count = settings.beginReadArray(QStringLiteral("RecentFiles"));
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        result.append(settings.value(QStringLiteral("file")).toString());
    }
    settings.endArray();
    return result;
}

void KMidimon::writeRecentFiles(const QStringList &files, QSettings &settings)
{
    const int count = files.size();
    settings.beginWriteArray(QStringLiteral("RecentFiles"));
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        settings.setValue(QStringLiteral("file"), files.at(i));
    }
    settings.endArray();
}

bool KMidimon::hasRecentFiles()
{
    QSettings settings;
    const int count = settings.beginReadArray(QStringLiteral("RecentFiles"));
    settings.endArray();
    return count > 0;
}

void KMidimon::prependToRecentFiles(const QString &fileName)
{
    QSettings settings;

    const QStringList oldRecentFiles = readRecentFiles(settings);
    QStringList recentFiles = oldRecentFiles;
    recentFiles.removeAll(fileName);
    recentFiles.prepend(fileName);
    if (oldRecentFiles != recentFiles)
        writeRecentFiles(recentFiles, settings);

    setRecentFilesVisible(!recentFiles.isEmpty());
}

void KMidimon::updateRecentFileActions()
{
    QSettings settings;
    const QStringList recentFiles = readRecentFiles(settings);
    const int count = qMin(int(MaxRecentFiles), recentFiles.size());
    int i = 0;
    for ( ; i < count; ++i) {
        const QString fileName = QFileInfo(recentFiles.at(i)).fileName();
        m_recentFileActs[i]->setText(tr("&%1 %2").arg(i + 1).arg(fileName));
        m_recentFileActs[i]->setData(recentFiles.at(i));
        m_recentFileActs[i]->setVisible(true);
    }
    for ( ; i < MaxRecentFiles; ++i)
        m_recentFileActs[i]->setVisible(false);
}

void KMidimon::openRecentFile()
{
    if (const QAction *action = qobject_cast<const QAction *>(sender()))
        open(action->data().toString());
}
