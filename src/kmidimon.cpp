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
#include <QTabBar>
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
    m_view->setSelectionMode(QAbstractItemView::NoSelection);
    m_adaptor = new SequencerAdaptor(this);
    m_adaptor->setModel(m_model);
    m_adaptor->updateModelClients();
    connect( m_model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                      SLOT(resizeColumns(QModelIndex,int,int)) );
    m_tabBar = new QTabBar(this);
    m_tabBar->setShape(QTabBar::RoundedNorth);
    connect( m_tabBar, SIGNAL(currentChanged(int)),
                       SLOT(tabIndexChanged(int)) );
    layout->addWidget(m_tabBar);
    layout->addWidget(m_view);
    vbox->setLayout(layout);
    setCentralWidget(vbox);
    setupActions();
    setAutoSaveSettings();
    readConfiguration();
    addNewTab(0);
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
    KStandardAction::openNew(this, SLOT(fileNew()), actionCollection());
    m_save = KStandardAction::saveAs(this, SLOT(fileSave()), actionCollection());
    m_prefs = KStandardAction::preferences(this, SLOT(preferences()), actionCollection());
    KStandardAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());

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
    m_connectAll->setText(i18n("&Connect All"));
    connect(m_connectAll, SIGNAL(triggered()), SLOT(connectAll()));
    actionCollection()->addAction("connect_all", m_connectAll);

    m_disconnectAll = new KAction(this);
    m_disconnectAll->setText(i18n("&Disconnect All")); //, KShortcut::null(),
    connect(m_disconnectAll, SIGNAL(triggered()), SLOT(disconnectAll()));
    actionCollection()->addAction( "disconnect_all", m_disconnectAll );

    m_configConns = new KAction(this);
    m_configConns->setText(i18n("Con&figure Connections"));
    connect(m_configConns, SIGNAL(triggered()), SLOT(configConnections()));
    actionCollection()->addAction("connections_dialog", m_configConns );

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
    m_adaptor->setResolution(config.readEntry("resolution", RESOLUTION));
    m_adaptor->setTempo(config.readEntry("tempo", TEMPO_BPM));
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
        was_running = m_adaptor->queue_running();
        if (was_running) stop();
        m_proxy->setFilterAlsaMsg(dlg.isRegAlsaMsg());
        m_proxy->setFilterChannelMsg(dlg.isRegChannelMsg());
        m_proxy->setFilterCommonMsg(dlg.isRegCommonMsg());
        m_proxy->setFilterRealTimeMsg(dlg.isRegRealTimeMsg());
        m_proxy->setFilterSysexMsg(dlg.isRegSysexMsg());
        m_model->setShowClientNames(dlg.showClientNames());
        m_model->setTranslateSysex(dlg.translateSysex());
        m_adaptor->setTempo(dlg.getTempo());
        m_adaptor->setResolution(dlg.getResolution());
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
    if (!m_adaptor->queue_running()) {
        m_adaptor->queue_start();
    }
    updateState("recording_state");
}

void KMidimon::stop()
{
    if (m_adaptor->queue_running()) {
        m_adaptor->queue_stop();
    }
    updateState("stopped_state");
}

void KMidimon::updateState(const QString newState)
{
    QString s(m_adaptor->queue_running() ? i18n("recording") : i18n("stopped"));
    setCaption(i18n("ALSA MIDI Monitor [%1]").arg(s));
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
    m_adaptor->connect_all();
}

void KMidimon::disconnectAll()
{
    m_adaptor->disconnect_all();
}

void KMidimon::configConnections()
{
    QStringList inputs = m_adaptor->inputConnections();
    QStringList subs = m_adaptor->list_subscribers();
    ConnectDlg dlg(this, inputs, subs);
    if (dlg.exec()) {
        QStringList desired = dlg.getSelected();
        subs = m_adaptor->list_subscribers();
        QStringList::ConstIterator i;
        for (i = subs.constBegin(); i != subs.constEnd(); ++i) {
            if (desired.contains(*i) == 0) {
                m_adaptor->disconnect_port(*i);
            }
        }
        for (i = desired.constBegin(); i != desired.constEnd(); ++i) {
            if (subs.contains(*i) == 0) {
                m_adaptor->connect_port(*i);
            }
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
}

void KMidimon::tabIndexChanged(int index)
{
    QVariant data = m_tabBar->tabData(index);
    //qDebug() << "current tab data: " << data.toInt();
    m_proxy->setFilterTrack(data.toInt());
}
