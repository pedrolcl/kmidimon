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
#include <QDirIterator>
#include <QActionGroup>
#include <QComboBox>
#include <QTextCodec>
#include <QDebug>
#include <drumstick/sequencererror.h>
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
#include "iconutils.h"
#include "helpwindow.h"

QString KMidimon::dataDirectory()
{
    QDir test(QApplication::applicationDirPath() + "/../share/kmidimon/");
    if (test.exists()) {
        return test.absolutePath();
    }
    QStringList candidates = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
    foreach(const QString& d, candidates) {
        test = QDir(d);
        if (test.exists()) {
            return d;
        }
    }
    return QString();
}

static QString trDirectory()
{
#if defined(TRANSLATIONS_EMBEDDED)
    return QLatin1String(":/");
#else
    QDir test(KMidimon::dataDirectory() + "/translations/" );
    if (test.exists()) {
        return test.absolutePath();
    }
    return QString();
#endif
}

static QString trQtDirectory()
{
#if defined(TRANSLATIONS_EMBEDDED)
    return QLatin1String(":/");
#else
    #if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    #else
        return QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    #endif
#endif
}

KMidimon::KMidimon() :
    QMainWindow(nullptr),
    m_state(InvalidState),
    m_adaptor(nullptr),
    m_trp(nullptr),
    m_trq(nullptr),
    m_currentLang(nullptr)
{
    QString lang = configuredLanguage();
    QLocale locale;
    if (!lang.isEmpty()) {
        locale = QLocale(lang);
    }
    if (locale.language() != QLocale::C && locale.language() != QLocale::English) {
        m_trq = new QTranslator(this);
        if (m_trq->load(locale, QLatin1String("qt"), QLatin1String("_"), trQtDirectory())) {
            QApplication::installTranslator(m_trq);
        } else {
            qWarning() << "Failure loading Qt translations for" << lang
                       << "from" << trQtDirectory();
            delete m_trq;
        }
        m_trp = new QTranslator(this);
        if (m_trp->load(locale, QLatin1String("kmidimon"), QLatin1String("_"), trDirectory())) {
            QApplication::installTranslator(m_trp);
        } else {
            qWarning() << "Failure loading program translations for" << lang
                       << "from" << trQtDirectory();
            delete m_trp;
        }
    }

    m_ui = new Ui::KMidimonWin();
    m_ui->setupUi(this);
    IconUtils::SetWindowIcon(this);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    m_useFixedFont = false;
    m_autoResizeColumns = false;
    m_requestRealtimePrio = false;
    m_model = new SequenceModel(this);
    m_proxy = new ProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_filter = new EventFilter(this);
    m_model->setFilter(m_filter);
    m_proxy->setFilter(m_filter);
    m_helpWindow = new HelpWindow(this);
    m_view = new QTreeView(this);
    m_view->setWhatsThis(tr("The events list"));
    m_view->setRootIsDecorated(false);
    m_view->setAlternatingRowColors(true);
    m_view->setModel(m_proxy);
    m_view->setSortingEnabled(false);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    setAcceptDrops(true);
    connect( m_view->selectionModel(),
             &QItemSelectionModel::currentChanged,
             this, &KMidimon::slotCurrentChanged );
    try {
        m_adaptor = new SequencerAdaptor(this);
        m_adaptor->setModel(m_model);
        m_adaptor->updateModelClients();
        connect( m_model, &QAbstractItemModel::rowsInserted,
                          this, &KMidimon::modelRowsInserted );
        connect( m_adaptor, &SequencerAdaptor::signalTicks, this, &KMidimon::slotTicks);
        connect( m_adaptor, &SequencerAdaptor::finished, this, &KMidimon::songFinished);
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
        connect( m_tabBar, &QTabBar::tabCloseRequested, this, &KMidimon::deleteTrack );
#endif
//        connect( m_tabBar, SIGNAL(newTabRequest()),
//                           SLOT(addTrack()) );
        connect( m_tabBar, &QTabBar::tabBarDoubleClicked, this, &KMidimon::changeTrack );
        connect( m_tabBar, &QTabBar::currentChanged, this, &KMidimon::tabIndexChanged );
        layout->addWidget(m_tabBar);
        layout->addWidget(m_view);
        vbox->setLayout(layout);
        setCentralWidget(vbox);
        setupActions();
        readConfiguration();
        createLanguageMenu();
        translateActions();
        applyVisualStyle();
        fileNew();
        record();
    } catch (drumstick::ALSA::SequencerError& ex) {
        QString errorstr = tr("Fatal error from the ALSA sequencer. "
            "This usually happens when the kernel doesn't have ALSA support, "
            "or the device node (/dev/snd/seq) doesn't exists, "
            "or the kernel module (snd_seq) is not loaded. "
            "Please check your ALSA/MIDI configuration. Returned error was: %1")
                .arg(ex.qstrError());
        QMessageBox::critical(nullptr, tr("Error"), errorstr);
        close();
    }
    m_helpWindow->applySettings();
}

void KMidimon::translateActions()
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
    m_new->setText(tr("&New"));
    m_new->setWhatsThis(tr("Clear the current data and start a new empty session"));
    m_open->setText(tr("&Open"));
    m_open->setWhatsThis(tr("Open a disk file"));
    m_save->setText(tr("&Save"));
    m_save->setWhatsThis(tr("Store the session data on a disk file"));
    m_quit->setText(tr("Quit"));
    m_quit->setWhatsThis(tr("Exit the application"));
    m_fileInfo->setText(tr("Sequence Info"));
    m_fileInfo->setWhatsThis(tr("Display information about the loaded sequence"));
    m_prefs->setText(tr("Preferences"));
    m_prefs->setWhatsThis(tr("Configure the program setting several preferences"));
    m_rewind->setText(tr("Backward","player skip backward"));
    m_rewind->setWhatsThis(tr("Move the playback position to the first event"));
    m_play->setText(tr("&Play"));
    m_play->setWhatsThis(tr("Start playback of the current session"));
    m_pause->setText(tr("Pause"));
    m_pause->setWhatsThis(tr("Pause the playback"));
    m_forward->setText(tr("Forward","player skip forward"));
    m_forward->setWhatsThis(tr("Move the playback position to the last event"));
    m_record->setText(tr("Record"));
    m_record->setWhatsThis(tr("Append new recorded events to the current session"));
    m_stop->setText( tr("Stop") );
    m_stop->setWhatsThis(tr("Stop playback or recording"));
    m_tempoSlider->setText(tr("Scale Tempo"));
    m_tempoSlider->setWhatsThis(tr("Display a slider to scale the tempo between 50% and 200%"));
    m_tempo100->setText(tr("Reset Tempo"));
    m_tempo100->setWhatsThis(tr("Reset the tempo scale to 100%"));
    m_connectAll->setText(tr("Connect All Inputs"));
    m_connectAll->setWhatsThis(tr("Connect all readable MIDI ports"));
    m_disconnectAll->setText(tr("Disconnect All Inputs"));
    m_disconnectAll->setWhatsThis(tr("Disconnect all input MIDI ports"));
    m_configConns->setText(tr("Configure Connections"));
    m_configConns->setWhatsThis(tr("Open the Connections dialog"));
    m_resizeColumns->setText(tr("Resize columns"));
    m_resizeColumns->setWhatsThis(tr("Resize the columns width to fit it's contents"));
    m_createTrack->setText(tr("Add Track View"));
    m_createTrack->setWhatsThis(tr("Create a new tab/track view"));
    m_deleteTrack->setText(tr("Delete Track View"));
    m_deleteTrack->setWhatsThis(tr("Delete the tab/track view"));
    m_changeTrack->setText(tr("Change Track View"));
    m_changeTrack->setWhatsThis(tr("Change the track number of the view"));
    m_muteTrack->setText(tr("Mute Track"));
    m_muteTrack->setWhatsThis(tr("Mute (silence) the track"));
    m_recentFiles->setTitle(tr("Recent files"));
    m_menuTracks->setTitle(tr("Tracks"));
    m_menuColumns->setTitle(tr("Show Columns"));
    for ( int i = 0; i < COLUMN_COUNT; ++i ) {
        m_popupAction[i]->setText(columnName[i]);
    }
    m_combolbl->setText(tr("Text Encoding:"));
}

void KMidimon::translateTabs()
{
    for(int i = 0; i < m_tabBar->count(); ++i) {
        int track = m_tabBar->tabData(i).toInt();
        m_tabBar->setTabText(i, tr("Track %1","song track").arg(track));
        m_tabBar->setTabWhatsThis(i, tr("Track %1 View Selector","track selector").arg(track));
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

    m_new = new QAction(this);
    m_new->setIcon(IconUtils::GetIcon("document-new"));
    connect(m_new, &QAction::triggered, this, &KMidimon::fileNew);
    m_ui->menuFile->addAction(m_new);
    m_ui->toolBar->addAction(m_new);

    m_open = new QAction(this);
    m_open->setIcon(IconUtils::GetIcon("document-open"));
    connect(m_open, &QAction::triggered, this, &KMidimon::fileOpen);
    m_ui->menuFile->addAction(m_open);
    m_ui->toolBar->addAction(m_open);

    m_recentFiles = m_ui->menuFile->addMenu(IconUtils::GetIcon("document-open-recent"), tr("Recent files"));
    connect(m_recentFiles, &QMenu::aboutToShow, this, &KMidimon::updateRecentFileActions);
    m_recentFileSubMenuAct = m_recentFiles->menuAction();

    for (int i = 0; i < MaxRecentFiles; ++i) {
        m_recentFileActs[i] = m_recentFiles->addAction(QString(), this, &KMidimon::openRecentFile);
        m_recentFileActs[i]->setVisible(false);
    }

    m_recentFileSeparator = m_ui->menuFile->addSeparator();
    setRecentFilesVisible(KMidimon::hasRecentFiles());

    m_save = new QAction(this);
    m_save->setIcon(IconUtils::GetIcon("document-save"));
    connect(m_save, &QAction::triggered, this, &KMidimon::fileSave);
    m_ui->menuFile->addAction(m_save);
    m_ui->toolBar->addAction(m_save);

    m_fileInfo = new QAction(this);
    m_fileInfo->setIcon(IconUtils::GetIcon("dialog-information"));
    connect(m_fileInfo, &QAction::triggered, this, &KMidimon::songFileInfo);
    m_ui->menuFile->addAction(m_fileInfo);
    m_ui->menuFile->addSeparator();

    m_quit = new QAction(this);
    m_quit->setIcon(IconUtils::GetIcon("application-exit"));
    connect(m_quit, &QAction::triggered, this, &QWidget::close);
    m_ui->menuFile->addAction(m_quit);
    m_ui->toolBar->addAction(m_quit);

    m_prefs = new QAction(this);
    m_prefs->setIcon(IconUtils::GetIcon("configure"));
    connect(m_prefs, &QAction::triggered, this, &KMidimon::preferences);
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
    m_rewind->setIcon(IconUtils::GetIcon("media-skip-backward"));
    connect(m_rewind, &QAction::triggered, this, &KMidimon::rewind);
    m_ui->menuControl->addAction(m_rewind);
    m_ui->toolBar->addAction(m_rewind);

    m_play = new QAction(this);
    m_play->setShortcut( Qt::Key_MediaPlay );
    m_play->setIcon(IconUtils::GetIcon("media-playback-start"));
    connect(m_play, &QAction::triggered, this, &KMidimon::play);
    m_ui->menuControl->addAction(m_play);
    m_ui->toolBar->addAction(m_play);

    m_pause = new QAction(this);
    m_pause->setCheckable(true);
    m_pause->setIcon(IconUtils::GetIcon("media-playback-pause"));
    connect(m_pause, &QAction::triggered, this, &KMidimon::pause);
    m_ui->menuControl->addAction(m_pause);
    m_ui->toolBar->addAction(m_pause);

    m_forward = new QAction(this);
    m_forward->setIcon(IconUtils::GetIcon("media-skip-forward"));
    connect(m_forward, &QAction::triggered, this, &KMidimon::forward);
    m_ui->menuControl->addAction(m_forward);
    m_ui->toolBar->addAction(m_forward);

    m_record = new QAction(this);
    m_record->setIcon(IconUtils::GetIcon("media-record"));
    m_record->setShortcut( Qt::Key_MediaRecord );
    connect(m_record, &QAction::triggered, this, &KMidimon::record);
    m_ui->menuControl->addAction(m_record);
    m_ui->toolBar->addAction(m_record);

    m_stop = new QAction(this);
    m_stop->setIcon(IconUtils::GetIcon("media-playback-stop"));
    m_stop->setShortcut( Qt::Key_MediaStop );
    connect(m_stop, &QAction::triggered, this, &KMidimon::stop);
    m_ui->menuControl->addAction(m_stop);
    m_ui->toolBar->addAction(m_stop);

    m_ui->menuControl->addSeparator();

    m_tempoSlider = new PlayerPopupSliderAction( this, SLOT(tempoSlider(int)), this );
    m_tempoSlider->setIcon(IconUtils::GetIcon("chronometer"));
    m_ui->menuControl->addAction(m_tempoSlider);
    m_ui->toolBar->addAction(m_tempoSlider);

    m_tempo100 = new QAction(this);
    m_tempo100->setIcon(IconUtils::GetIcon("player-time"));
    connect(m_tempo100, &QAction::triggered, this, &KMidimon::tempoReset);
    m_ui->menuControl->addAction(m_tempo100);
    m_ui->toolBar->addAction(m_tempo100);

    m_connectAll = new QAction(this);
    connect(m_connectAll, &QAction::triggered, this, &KMidimon::connectAll);
    m_ui->menuConnections->addAction(m_connectAll);

    m_disconnectAll = new QAction(this);
    connect(m_disconnectAll, &QAction::triggered, this, &KMidimon::disconnectAll);
    m_ui->menuConnections->addAction(m_disconnectAll);

    m_ui->menuConnections->addSeparator();

    m_configConns = new QAction(this);
    connect(m_configConns, &QAction::triggered, this, &KMidimon::configConnections);
    m_ui->menuConnections->addAction(m_configConns);

    m_popup = new QMenu(this);

    m_resizeColumns = new QAction(this);
    connect(m_resizeColumns, &QAction::triggered, this, &KMidimon::resizeAllColumns);
    m_popup->addAction(m_resizeColumns);

    m_menuTracks = m_popup->addMenu(tr("Tracks"));

    m_createTrack = new QAction(this);
    connect(m_createTrack, &QAction::triggered, this, &KMidimon::addTrack);
    m_menuTracks->addAction(m_createTrack);

    m_deleteTrack = new QAction(this);
    connect(m_deleteTrack, &QAction::triggered, this, &KMidimon::deleteCurrentTrack);
    m_menuTracks->addAction(m_deleteTrack);

    m_changeTrack = new QAction(this);
    connect(m_changeTrack, &QAction::triggered, this, &KMidimon::changeCurrentTrack);
    m_menuTracks->addAction(m_changeTrack);

    m_muteTrack = new QAction(this);
    m_muteTrack->setCheckable(true);
    connect(m_muteTrack, &QAction::triggered, this, &KMidimon::muteCurrentTrack);
    m_menuTracks->addAction(m_muteTrack);

    m_menuColumns = m_popup->addMenu(tr("Show Columns"));

    for ( int i = 0; i < COLUMN_COUNT; ++i ) {
        m_popupAction[i] = new QAction(columnName[i], this);
        m_popupAction[i]->setCheckable(true);
        m_popupAction[i]->setChecked(true);
        m_popupAction[i]->setWhatsThis(tr("Toggle the %1 column").arg(columnName[i]));
        connect(m_popupAction[i], &QAction::triggered, this, [=] {toggleColumn(i);});
        m_menuColumns->addAction(m_popupAction[i]);
    }

    QMenu* filtersMenu = m_filter->buildMenu(this);
    m_popup->addMenu( filtersMenu );
    connect(m_filter, &EventFilter::filterChanged, m_proxy, &QSortFilterProxyModel::invalidate);
    menuBar()->insertMenu( menuBar()->actions().last(), filtersMenu );

    m_ui->toolBar->addSeparator();
    m_combolbl = new QLabel(tr("Text Encoding:"));
    m_ui->toolBar->addWidget(m_combolbl);
    m_textcodecs = new QComboBox(this);
    m_ui->toolBar->addWidget(m_textcodecs);
    initTextCodecs();
    connect( m_textcodecs, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &KMidimon::textCodecChanged);

    m_ui->actionAbout_Qt->setIcon(QIcon(":/qt-project.org/qmessagebox/images/qtlogo-64.png"));
    m_ui->actionAbout->setIcon(QIcon(IconUtils::GetPixmap(":/icons/midi/icon32.png")));
    m_ui->actionContents->setIcon(IconUtils::GetIcon("help-contents"));
    connect( m_ui->actionAbout, &QAction::triggered, this, &KMidimon::about );
    connect( m_ui->actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt );
    connect( m_ui->actionContents, &QAction::triggered, this, &KMidimon::help );
    connect( m_ui->actionWeb_Site, &QAction::triggered, this, &KMidimon::slotOpenWebSite );
}

void KMidimon::about()
{
    About dlg(this);
    dlg.exec();
}

void KMidimon::help()
{
    m_helpWindow->show();
}

void KMidimon::slotOpenWebSite()
{
    QUrl url(QStringLiteral("https://kmidimon.sourceforge.io"));
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
    QString loadingMsg;
    try {
        QFileInfo finfo(fileName);
        if (finfo.exists()) {
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
            m_pd->setCancelButton(nullptr);
            m_pd->setMinimumDuration(500);
            m_pd->setRange(0, finfo.size());
            m_pd->setValue(0);
            connect( m_model, &SequenceModel::loadProgress, m_pd.data(), &QProgressDialog::setValue );
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
            for (int i = 0; i < ntrks; i++) {
                int tab = m_model->getTrackForIndex(i);
                addNewTab(tab);
            }
            m_tabBar->setCurrentIndex(0);
            m_proxy->setFilterTrack(0);
            m_model->setCurrentTrack(0);
            for (int i = 0; i < COLUMN_COUNT; ++i)
                m_view->resizeColumnToContents(i);
            tempoReset();
            prependToRecentFiles(fileName);
            updateCaption();
            loadingMsg = m_model->getLoadingMessages();
        } else {
            loadingMsg = tr("file not found");
        }
    } catch (...) {
        m_model->clear();
    }
    m_view->blockSignals(false);
    delete m_pd;
    m_pd = nullptr;
    if (!loadingMsg.isEmpty()) {
        loadingMsg.insert(0, tr("Warning, this file may be non-standard or damaged.<br/>"));
        QMessageBox::warning(this, tr("File parsing error"), loadingMsg);
    }
}

void KMidimon::fileOpen()
{
    QFileDialog fd(this, tr("Open MIDI file"));
    QStringList filters{"audio/midi","audio/cakewalk","audio/x-midi","application/x-riff"};
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
    QStringList filters = {tr("Plain text files (*.txt)"), tr("MIDI files (*.mid)")};
    fd.setNameFilters(filters);
    fd.setFileMode(QFileDialog::AnyFile);
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.selectNameFilter(filters[0]);
    fd.setDefaultSuffix("txt");
    connect(&fd, &QFileDialog::filterSelected, this, [&fd](const QString& filter){
        if (filter.contains("*.mid")) {
            fd.setDefaultSuffix("mid");
        } else {
            fd.setDefaultSuffix("txt");
        }
    });
    if (fd.exec()) {
        auto path = fd.selectedFiles().constFirst();
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
    if (m_adaptor == nullptr) return;
    config.beginGroup("Settings");
    config.setValue("geometry", saveGeometry());
    config.setValue("windowState", saveState());
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
    config.setValue("language", m_language);
    config.setValue("style", m_style);
    config.setValue("dark_mode", m_darkMode);
    config.setValue("internal_icons", m_internalIcons);
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
    restoreGeometry(config.value("geometry").toByteArray());
    restoreState(config.value("windowState").toByteArray());
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
    m_model->setInstrumentName(config.value("instrument", QLatin1String("General MIDI")).toString());
    setTextCodec(config.value("encoding", QLatin1String("latin1")).toString());
    m_autoResizeColumns = config.value("auto_resize", false).toBool();
    m_defaultResolution = config.value("resolution", RESOLUTION).toInt();
    m_defaultTempo = config.value("tempo", TEMPO_BPM).toInt();
    m_requestRealtimePrio = config.value("realtime_prio", false).toBool();
    m_adaptor->setResolution(m_defaultResolution);
    m_adaptor->setTempo(m_defaultTempo);
    m_adaptor->queue_set_tempo();
    m_adaptor->setRequestRealtime(m_requestRealtimePrio);
    setFixedFont(config.value("fixed_font", true).toBool());
    m_style = config.value("style", "Fusion").toString();
    m_darkMode = config.value("dark_mode", false).toBool();
    m_internalIcons = config.value("internal_icons", false).toBool();
    for (i = 0; i < COLUMN_COUNT; ++i) {
        status = config.value(QString("show_column_%1").arg(i), true).toBool();
        setColumnStatus(i, status);
    }
    m_outputConn = config.value("output_connection", QString()).toString();
    m_adaptor->connect_output(m_outputConn);
    m_filter->loadConfiguration();
    m_helpWindow->setIcons(m_internalIcons);
}

void KMidimon::preferences()
{
    int i;
    bool was_running;
    QScopedPointer<ConfigDialog> dlg(new ConfigDialog(this));
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
    dlg->setUseFixedFont(getFixedFont());
    dlg->setResizeColumns(m_autoResizeColumns);
    dlg->setStyle(m_style);
    dlg->setDarkMode(m_darkMode);
    dlg->setInternalIcons(m_internalIcons);
    for (i = 0; i < COLUMN_COUNT; ++i) {
        dlg->setShowColumn(i, m_popupAction[i]->isChecked());
    }
    if (dlg->exec() == QDialog::Accepted) {
        if (dlg != nullptr) {
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
            m_autoResizeColumns = dlg->resizeColumns();
            m_defaultTempo = dlg->getTempo();
            m_defaultResolution = dlg->getResolution();
            setFixedFont(dlg->useFixedFont());
            m_style = dlg->getStyle();
            m_darkMode = dlg->getDarkMode();
            m_internalIcons = dlg->getInternalIcons();
            m_helpWindow->setIcons(m_internalIcons);
            for (i = 0; i < COLUMN_COUNT; ++i) {
                setColumnStatus(i, dlg->showColumn(i));
            }
            m_requestRealtimePrio = dlg->requestRealtime();
            m_adaptor->setRequestRealtime(m_requestRealtimePrio);
            applyVisualStyle();
            if (was_running) record();
            m_helpWindow->applySettings();
        }
    }
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
    setWindowTitle(tr("%1 [%2]").arg(name, m_currentState));
}

void KMidimon::updateState(PlayerState newState)
{
    if (m_state == newState)
        return;
    m_state = newState;
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
    QScopedPointer<ConnectDlg> dlg(new ConnectDlg(this, inputs, subs, outputs, m_outputConn));
    dlg->setThruEnabled(m_adaptor->isThruEnabled());
    if (dlg->exec() == QDialog::Accepted) {
        if (dlg != nullptr) {
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
            m_adaptor->setThruEnabled(dlg->isThruEnabled());
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
    /* row is an index to the Song (QList<SequencerItems>), but
     * SequenceModel's items are stored in a Song, so ... */
    QModelIndex index = m_model->getRowIndex(row);
    if (index.isValid()) {
        QModelIndex vidx = m_proxy->mapFromSource(index);
        if (vidx.isValid() && (m_state == PlayingState)) {
            m_view->setCurrentIndex(vidx);
        }
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
        QLocale locale(m_language);
        QFileInfo finfo(m_currentFile);
        infostr = tr( "File: <b>%1</b><br/>"
                      "Date: <b>%2</b><br/>"
                      "Format: <b>%3</b><br/>"
                      "Number of tracks: <b>%4</b><br/>"
                      "Number of events: <b>%5</b><br/>"
                      "Division: <b>%6 ppq</b><br/>"
                      "Initial tempo: <b>%7 bpm</b><br/>"
                      "Duration: <b>%8</b><br/> %9").arg(
                      finfo.fileName(),
                      locale.toString(finfo.lastModified(), QLocale::LongFormat),
                      m_model->getFileFormat(),
                      QString::number( m_model->getSMFTracks() ),
                      QString::number( m_model->getSong()->size() ),
                      QString::number( m_model->getSMFDivision() ),
                      QString::number( m_model->getInitialTempo() ),
                      m_model->getDuration(),
                      m_model->getMetadataInfo());
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
    if (song != nullptr) {
        bool newState = !song->mutedState(track);
        song->setMutedState(track, newState);
        m_tabBar->setTabIcon(tabIndex, newState ? IconUtils::GetIcon("player-volume-muted") : QIcon());
        if (newState) {
            m_adaptor->removeTrackEvents(track);
            m_adaptor->shutupSound();
        }
    }
}

void KMidimon::muteCurrentTrack()
{
    muteTrack(m_tabBar->currentIndex());
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
            open(urls.first().toLocalFile());
        event->accept();
    }
}

QString KMidimon::configuredLanguage()
{
    if (m_language.isEmpty()) {
        QLocale loc;
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
        QString defLang = loc.name().split('_').at(0).toLower();
#else
        QString defLang = QLocale::languageToCode(loc.language());
#endif
        if (defLang.isEmpty() || defLang == "en") {
            defLang = "C";
        }
        QSettings settings;
        settings.beginGroup("Settings");
        m_language = settings.value("language", defLang).toString();
        settings.endGroup();
    }
    return m_language;
}

void KMidimon::slotSwitchLanguage(QAction *action)
{
    QString lang = action->data().toString();
    QLocale qlocale(lang);
    QString localeName = lang == "C" ? QLocale::languageToString(QLocale::English) : qlocale.nativeLanguageName();
    if ( QMessageBox::question (this, tr("Language Changed"),
            tr("The language for this application is going to change to %1. "
               "Do you want to continue?").arg(localeName),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes )
    {
        m_language = lang;
        retranslateUi();
    } else {
        if (m_currentLang == nullptr) {
            m_currentLang = action;
        }
        m_currentLang->setChecked(true);
    }
}

void KMidimon::createLanguageMenu()
{
    QString currentLang = configuredLanguage();
    if (currentLang.isEmpty()) {
        QLocale loc;
        if (loc.language() == QLocale::C || loc.language() == QLocale::English)
            currentLang = "C";
        else
            currentLang = QLocale().name().left(2);
    }
    QActionGroup *languageGroup = new QActionGroup(this);
    connect(languageGroup, &QActionGroup::triggered,
            this, &KMidimon::slotSwitchLanguage);
    QStringList locales;
    locales << "C";
    QDirIterator it(trDirectory(), {"kmidimon*.qm"}, QDir::NoFilter, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        QFileInfo f(it.next());
        QString locale = f.fileName();
        if (locale.startsWith("kmidimon_")) {
            locale.remove(0, 9).truncate(locale.lastIndexOf('.'));
            locales << locale;
        }
    }
    locales.sort();
    m_ui->menuLanguage->clear();
    foreach (const QString& loc, locales) {
        QLocale qlocale(loc);
        QString localeName = loc == "C" ? QLocale::languageToString(QLocale::English) : qlocale.nativeLanguageName();
        QAction *action = new QAction(localeName.section(" ", 0, 0), this);
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
    QLocale locale;
    QString lang = configuredLanguage();
    if (!lang.isEmpty()) {
        locale = QLocale(lang);
    }
    if (m_trq) {
        QCoreApplication::removeTranslator(m_trq);
        delete m_trq;
    }
    if (m_trp) {
        QCoreApplication::removeTranslator(m_trp);
        delete m_trp;
    }
    if (locale.language() != QLocale::C && locale.language() != QLocale::English) {
        m_trq = new QTranslator(this);
        if (m_trq->load(locale, QLatin1String("qt"), QLatin1String("_"), trQtDirectory())) {
            QCoreApplication::installTranslator(m_trq);
        } else {
            qWarning() << "Failure loading Qt translations for" << lang
                       << "from" << trQtDirectory();
            delete m_trq;
        }
        m_trp = new QTranslator(this);
        if (m_trp->load(locale, QLatin1String("kmidimon"), QLatin1String("_"), trDirectory())) {
            QCoreApplication::installTranslator(m_trp);
        } else {
            qWarning() << "Failure loading program translations for" << lang
                       << "from" << trDirectory();
            delete m_trp;
        }
    }
    m_ui->retranslateUi(this);
    createLanguageMenu();
    translateActions();
    translateTabs();
    m_filter->retranslateMenu();
    m_helpWindow->retranslateUi();
}

void KMidimon::applyVisualStyle()
{
    static QPalette light = qApp->palette();
    static QPalette dark(QColor(0x30,0x30,0x30));
    qApp->setPalette( m_darkMode ? dark : light );
    qApp->setStyle( m_style );
    refreshIcons();
}

void KMidimon::refreshIcons()
{
    m_new->setIcon(IconUtils::GetIcon("document-new", m_internalIcons));
    m_open->setIcon(IconUtils::GetIcon("document-open", m_internalIcons));
    m_recentFiles->setIcon(IconUtils::GetIcon("document-open-recent", m_internalIcons));
    m_save->setIcon(IconUtils::GetIcon("document-save", m_internalIcons));
    m_fileInfo->setIcon(IconUtils::GetIcon("dialog-information", m_internalIcons));
    m_quit->setIcon(IconUtils::GetIcon("application-exit", m_internalIcons));
    m_prefs->setIcon(IconUtils::GetIcon("configure", m_internalIcons));
    m_rewind->setIcon(IconUtils::GetIcon("media-skip-backward", m_internalIcons));
    m_play->setIcon(IconUtils::GetIcon("media-playback-start", m_internalIcons));
    m_pause->setIcon(IconUtils::GetIcon("media-playback-pause", m_internalIcons));
    m_forward->setIcon(IconUtils::GetIcon("media-skip-forward", m_internalIcons));
    m_record->setIcon(IconUtils::GetIcon("media-record", m_internalIcons));
    m_stop->setIcon(IconUtils::GetIcon("media-playback-stop", m_internalIcons));
    m_tempoSlider->setIcon(IconUtils::GetIcon("chronometer", m_internalIcons));
    m_tempo100->setIcon(IconUtils::GetIcon("player-time", m_internalIcons));
    m_ui->actionContents->setIcon(IconUtils::GetIcon("help-contents", m_internalIcons));
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
        QFileInfo fi(recentFiles.at(i));
        if (fi.exists()) {
            const QString fileName = fi.fileName();
            m_recentFileActs[i]->setText(tr("&%1 %2").arg(i + 1).arg(fileName));
            m_recentFileActs[i]->setData(recentFiles.at(i));
            m_recentFileActs[i]->setVisible(true);
        }
    }
    for ( ; i < MaxRecentFiles; ++i)
        m_recentFileActs[i]->setVisible(false);
}

void KMidimon::openRecentFile()
{
    if (const QAction *action = qobject_cast<const QAction *>(sender()))
        open(action->data().toString());
}

void KMidimon::initTextCodecs()
{
    m_textcodecs->clear();
    m_textcodecs->addItem(tr("Default ( Latin1 )", "@item:inlistbox Default MIDI text encoding"));
    QStringList encodings;
    foreach (auto m,  QTextCodec::availableMibs())
    {
        if (QTextCodec* c = QTextCodec::codecForMib(m))
        {
            QString name = c->name();
            if (!encodings.contains(name))
                encodings.append(name);
        }
    }
    encodings.sort();
    m_textcodecs->addItems(encodings);
    m_textcodecs->setCurrentIndex(-1);
}

void KMidimon::setTextCodec(const QString &encoding)
{
    int index = m_textcodecs->findText( encoding );
    if (index == -1 || encoding.isEmpty() || (encoding == QLatin1String("latin1"))) {
        index = 0;
    }
    m_textcodecs->setCurrentIndex( index );
}

void KMidimon::textCodecChanged(int index)
{
    QString codecName = index <= 0 ? QLatin1String("latin1") : m_textcodecs->itemText(index);
    m_model->setEncoding(codecName);
    m_proxy->invalidate();
}
