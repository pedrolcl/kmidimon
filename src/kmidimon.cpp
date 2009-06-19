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

#include "kmidimon.h"
#include "configdialog.h"
#include "connectdlg.h"
#include "sequencemodel.h"
#include "proxymodel.h"

KMidimon::KMidimon() :
    KXmlGuiWindow(0)
{
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    m_useFixedFont = false;
    m_orderedEvents = false;
    m_mapper = new QSignalMapper(this);
    m_model = new SequenceModel(this);
    m_model->setOrdered(m_orderedEvents);
    m_proxy = new ProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_view = new QTreeView(this);
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
    record();
}

void KMidimon::setupActions()
{
    const QString columnName[COLUMN_COUNT] = {
            i18n("T&icks"),
            i18n("&Time"),
            i18n("&Source"),
            i18n("&Event Kind"),
            i18n("&Channel"),
            i18n("Data &1"),
            i18n("Data &2")
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
    KStandardAction::quit(kapp, SLOT(quit()), actionCollection());
    KStandardAction::open(this, SLOT(fileOpen()), actionCollection());
    KStandardAction::openNew(this, SLOT(fileNew()), actionCollection());
    m_save = KStandardAction::saveAs(this, SLOT(fileSave()), actionCollection());
    m_prefs = KStandardAction::preferences(this, SLOT(preferences()), actionCollection());
    KStandardAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());

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
    connect(m_play, SIGNAL(triggered()), SLOT(play()));
    actionCollection()->addAction("play", m_play);

    m_pause = new KToggleAction(this);
    m_pause->setText(i18n("Pause"));
    m_pause->setIcon(KIcon("media-playback-pause"));
    connect(m_pause, SIGNAL(triggered()), SLOT(pause()));
    actionCollection()->addAction("pause", m_pause);

    m_forward = new KAction(this);
    m_forward->setText(i18n("Forward"));
    m_forward->setIcon(KIcon("media-skip-forward"));
    connect(m_forward, SIGNAL(triggered()), SLOT(forward()));
    actionCollection()->addAction("forward", m_forward);

    m_rewind = new KAction(this);
    m_rewind->setText(i18n("Backward"));
    m_rewind->setIcon(KIcon("media-skip-backward"));
    connect(m_rewind, SIGNAL(triggered()), SLOT(rewind()));
    actionCollection()->addAction("rewind", m_rewind);

    m_record = new KAction(this);
    m_record->setText(i18n("&Record"));
    m_record->setIcon(KIcon("media-record"));
    m_record->setShortcut( Qt::Key_R );
    connect(m_record, SIGNAL(triggered()), SLOT(record()));
    actionCollection()->addAction("record", m_record);

    m_stop = new KAction(this);
    m_stop->setText( i18n("&Stop") );
    m_stop->setIcon(KIcon("media-playback-stop"));
    m_stop->setShortcut( Qt::Key_S );
    connect(m_stop, SIGNAL(triggered()), SLOT(stop()));
    actionCollection()->addAction("stop", m_stop);

    m_connectAll = new KAction(this);
    m_connectAll->setText(i18n("&Connect All Inputs"));
    connect(m_connectAll, SIGNAL(triggered()), SLOT(connectAll()));
    actionCollection()->addAction("connect_all", m_connectAll);

    m_disconnectAll = new KAction(this);
    m_disconnectAll->setText(i18n("&Disconnect All Inputs")); //, KShortcut::null(),
    connect(m_disconnectAll, SIGNAL(triggered()), SLOT(disconnectAll()));
    actionCollection()->addAction( "disconnect_all", m_disconnectAll );

    m_configConns = new KAction(this);
    m_configConns->setText(i18n("Con&figure Connections"));
    connect(m_configConns, SIGNAL(triggered()), SLOT(configConnections()));
    actionCollection()->addAction("connections_dialog", m_configConns );

    m_createTrack = new KAction(this);
    m_createTrack->setText(i18n("&Add Track View"));
    connect(m_createTrack, SIGNAL(triggered()), SLOT(addTrack()));
    actionCollection()->addAction("add_track", m_createTrack );

    m_changeTrack = new KAction(this);
    m_changeTrack->setText(i18n("&Change Track View"));
    connect(m_changeTrack, SIGNAL(triggered()), SLOT(changeCurrentTrack()));
    actionCollection()->addAction("change_track", m_changeTrack );

    m_deleteTrack = new KAction(this);
    m_deleteTrack->setText(i18n("&Delete Track View"));
    connect(m_deleteTrack, SIGNAL(triggered()), SLOT(deleteCurrentTrack()));
    actionCollection()->addAction("delete_track", m_deleteTrack );

    for(int i = 0; i < COLUMN_COUNT; ++i ) {
        m_popupAction[i] = new KToggleAction(columnName[i], this);
        connect(m_popupAction[i], SIGNAL(triggered()), m_mapper, SLOT(map()));
        m_mapper->setMapping(m_popupAction[i], i);
        actionCollection()->addAction(actionName[i], m_popupAction[i]);
    }
    connect(m_mapper, SIGNAL(mapped(int)), SLOT(toggleColumn(int)));

    setStandardToolBarMenuEnabled(true);
    setupGUI();

    m_popup = static_cast <QMenu*>(guiFactory()->container("popup", this));
    Q_CHECK_PTR( m_popup );
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
    m_adaptor->setTempo(m_defaultTempo);
    m_adaptor->setResolution(m_defaultResolution);
    m_adaptor->queue_set_tempo();
}

void KMidimon::fileOpen()
{
    QString path = KFileDialog::getOpenFileName(KUrl(
            "kfiledialog:///MIDIMONITOR"),
            i18n("*.mid|MIDI files (*.mid)"), this,
            i18n("Open MIDI file"));
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
                if (m_model->getInitialTempo() > 0)
                    m_adaptor->setTempo(m_model->getInitialTempo());
                int ntrks = m_model->getSMFTracks();
                if (ntrks < 1) ntrks = 1;
                //if (ntrks > 8) ntrks = 8;
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
            delete m_pd;
        }
    }
}

void KMidimon::fileSave()
{
    QString path = KFileDialog::getSaveFileName(KUrl(
            "kfiledialog:///MIDIMONITOR"), i18n(
            "*.txt|Plain text files (*.txt)"), this, i18n(
            "Save MIDI monitor data"));
    if (!path.isNull()) {
        QFile file(path);
        file.open(QIODevice::WriteOnly);
        QTextStream stream(&file);
        m_model->saveToStream(stream);
        file.close();
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
    config.writeEntry("resolution", m_adaptor->getResolution());
    config.writeEntry("tempo", m_adaptor->getTempo());
    config.writeEntry("alsa", m_proxy->showAlsaMsg());
    config.writeEntry("channel", m_proxy->showChannelMsg());
    config.writeEntry("common", m_proxy->showCommonMsg());
    config.writeEntry("realtime", m_proxy->showRealTimeMsg());
    config.writeEntry("sysex", m_proxy->showSysexMsg());
    config.writeEntry("client_names", m_model->showClientNames());
    config.writeEntry("translate_sysex", m_model->translateSysex());
    config.writeEntry("fixed_font", getFixedFont());
    config.writeEntry("sort_events", orderedEvents());
    for (i = 0; i < COLUMN_COUNT; ++i) {
        config.writeEntry(QString("show_column_%1").arg(i),
                m_popupAction[i]->isChecked());
    }
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
    m_model->setShowClientNames(config.readEntry("client_names", false));
    m_model->setTranslateSysex(config.readEntry("translate_sysex", false));
    m_adaptor->setResolution(m_defaultResolution = config.readEntry("resolution", RESOLUTION));
    m_adaptor->setTempo(m_defaultTempo = config.readEntry("tempo", TEMPO_BPM));
    m_adaptor->queue_set_tempo();
    setFixedFont(config.readEntry("fixed_font", false));
    setOrderedEvents(config.readEntry("sort_events", false));
    for (i = 0; i < COLUMN_COUNT; ++i) {
        status = config.readEntry(QString("show_column_%1").arg(i), true);
        setColumnStatus(i, status);
    }
}

void KMidimon::preferences()
{
    int i;
    bool was_running;
    ConfigDialog dlg;

    dlg.setTempo(m_adaptor->getTempo());
    dlg.setResolution(m_adaptor->getResolution());
    dlg.setRegAlsaMsg(m_proxy->showAlsaMsg());
    dlg.setRegChannelMsg(m_proxy->showChannelMsg());
    dlg.setRegCommonMsg(m_proxy->showCommonMsg());
    dlg.setRegRealTimeMsg(m_proxy->showRealTimeMsg());
    dlg.setRegSysexMsg(m_proxy->showSysexMsg());
    dlg.setShowClientNames(m_model->showClientNames());
    dlg.setTranslateSysex(m_model->translateSysex());
    dlg.setUseFixedFont(getFixedFont());
    dlg.setOrderedEvents(orderedEvents());
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
        m_model->setShowClientNames(dlg.showClientNames());
        m_model->setTranslateSysex(dlg.translateSysex());
        m_adaptor->setTempo(m_defaultTempo = dlg.getTempo());
        m_adaptor->setResolution(m_defaultResolution = dlg.getResolution());
        m_adaptor->queue_set_tempo();
        setFixedFont(dlg.useFixedFont());
        setOrderedEvents(dlg.orderedEvents());
        for (i = 0; i < COLUMN_COUNT; ++i) {
            setColumnStatus(i, dlg.showColumn(i));
        }
        if (was_running) record();
    }
}

void KMidimon::record()
{
    m_adaptor->record();
    updateState("recording_state", i18n("recording"));
}

void KMidimon::stop()
{
    bool wasRecording = m_adaptor->isRecording();
    if (wasRecording | m_adaptor->isPlaying()) {
        m_adaptor->stop();
        if (wasRecording) m_model->sortSong();
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
    setCaption(i18n("ALSA MIDI Monitor [%1]").arg(stateName));
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
    QString out = m_adaptor->output_subscriber();
    ConnectDlg dlg(this, inputs, subs, outputs, out);
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
        if (newOut != out) {
            m_adaptor->disconnect_output(out);
            m_adaptor->connect_output(newOut);
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

void KMidimon::setOrderedEvents(bool newValue)
{
    if (m_orderedEvents != newValue) {
        m_orderedEvents = newValue;
        m_model->setOrdered(m_orderedEvents);
        fileNew();
    }
}

void KMidimon::resizeColumns(const QModelIndex&, int, int)
{
    for( int i = 0; i < COLUMN_COUNT; ++i)
        m_view->resizeColumnToContents(i);
    if (m_orderedEvents)
        m_view->scrollToBottom();
    else
        m_view->scrollToTop();
}

void KMidimon::addNewTab(int data)
{
    QString tabName = i18n("Track %1").arg(data);
    int i = m_tabBar->addTab(tabName);
    m_tabBar->setTabData(i, QVariant(data));
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
        QString tabName = i18n("Track %1").arg(track);
        m_tabBar->setTabData(tabIndex, track);
        m_tabBar->setTabText(tabIndex, tabName);
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
