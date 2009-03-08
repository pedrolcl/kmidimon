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

KMidimon::KMidimon() :
    KXmlGuiWindow(0)
{
    m_useFixedFont = false;
    m_sortEvents = false;
    m_model = new SequenceModel(this);
    m_model->setSorted(m_sortEvents);
    m_view = new QTreeView(this);
    m_view->setRootIsDecorated(false);
    m_view->setAlternatingRowColors(true);
    m_view->setModel(m_model);
    m_view->setSortingEnabled(false);
    m_view->setSelectionMode(QAbstractItemView::NoSelection);
    m_adaptor = new SequencerAdaptor(this);
    m_adaptor->setModel(m_model);
    connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                     SLOT(resizeColumns(QModelIndex,int,int)) );
    setCentralWidget(m_view);
    setupActions();
    setAutoSaveSettings();
    readConfiguration();
    record();
}

void KMidimon::setupActions()
{
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

    m_popupAction[0] = new KToggleAction(i18n("&Time"), this);
    connect(m_popupAction[0], SIGNAL(triggered()), SLOT(toggleColumn0()));
    actionCollection()->addAction("show_time", m_popupAction[0]);

    m_popupAction[1] = new KToggleAction(i18n("&Source"), this);
    connect(m_popupAction[1], SIGNAL(triggered()), SLOT(toggleColumn1()));
    actionCollection()->addAction("show_source", m_popupAction[1]);

    m_popupAction[2] = new KToggleAction( i18n("&Event Kind"), this );
    connect(m_popupAction[2], SIGNAL(triggered()), SLOT(toggleColumn2()));
    actionCollection()->addAction("show_kind", m_popupAction[2]);

    m_popupAction[3] = new KToggleAction( i18n("&Channel"), this );
    connect(m_popupAction[3], SIGNAL(triggered()), SLOT(toggleColumn3()));
    actionCollection()->addAction("show_channel", m_popupAction[3]);

    m_popupAction[4] = new KToggleAction( i18n("Data &1"), this );
    connect(m_popupAction[4], SIGNAL(triggered()), SLOT(toggleColumn4()));
    actionCollection()->addAction("show_data1", m_popupAction[4]);

    m_popupAction[5] = new KToggleAction( i18n("Data &2"), this );
    connect(m_popupAction[5], SIGNAL(triggered()), SLOT(toggleColumn5()));
    actionCollection()->addAction("show_data2", m_popupAction[5]);

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
    config.writeEntry("tick_time", m_adaptor->isTickTime());
    config.writeEntry("alsa", m_adaptor->isRegAlsaMsg());
    config.writeEntry("channel", m_adaptor->isRegChannelMsg());
    config.writeEntry("common", m_adaptor->isRegCommonMsg());
    config.writeEntry("realtime", m_adaptor->isRegRealTimeMsg());
    config.writeEntry("sysex", m_adaptor->isRegSysexMsg());
    config.writeEntry("client_names", m_adaptor->showClientNames());
    config.writeEntry("translate_sysex", m_adaptor->translateSysex());
    config.writeEntry("fixed_font", getFixedFont());
    config.writeEntry("sort_events", getSortEvents());
    for (i = 0; i < 6; ++i) {
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
    m_adaptor->setResolution(config.readEntry("resolution", RESOLUTION));
    m_adaptor->setTempo(config.readEntry("tempo", TEMPO_BPM));
    m_adaptor->setTickTime(config.readEntry("tick_time", true));
    m_adaptor->setRegAlsaMsg(config.readEntry("alsa", true));
    m_adaptor->setRegChannelMsg(config.readEntry("channel", true));
    m_adaptor->setRegCommonMsg(config.readEntry("common", true));
    m_adaptor->setRegRealTimeMsg(config.readEntry("realtime", true));
    m_adaptor->setRegSysexMsg(config.readEntry("sysex", true));
    m_adaptor->setShowClientNames(config.readEntry("client_names", false));
    m_adaptor->setTranslateSysex(config.readEntry("translate_sysex", false));
    m_adaptor->queue_set_tempo();
    m_adaptor->change_port_settings();
    setFixedFont(config.readEntry("fixed_font", false));
    setSortEvents(config.readEntry("sort_events", false));
    for (i = 0; i < 6; ++i) {
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
    if (m_adaptor->isTickTime()) {
        dlg.setMusicalTime();
    } else {
        dlg.setClockTime();
    }
    dlg.setRegAlsaMsg(m_adaptor->isRegAlsaMsg());
    dlg.setRegChannelMsg(m_adaptor->isRegChannelMsg());
    dlg.setRegCommonMsg(m_adaptor->isRegCommonMsg());
    dlg.setRegRealTimeMsg(m_adaptor->isRegRealTimeMsg());
    dlg.setRegSysexMsg(m_adaptor->isRegSysexMsg());
    dlg.setShowClientNames(m_adaptor->showClientNames());
    dlg.setTranslateSysex(m_adaptor->translateSysex());
    dlg.setUseFixedFont(getFixedFont());
    dlg.setSortEvents(getSortEvents());
    for (i = 0; i < 6; ++i) {
        dlg.setShowColumn(i, m_popupAction[i]->isChecked());
    }
    if (dlg.exec()) {
        was_running = m_adaptor->queue_running();
        if (was_running) stop();
        m_adaptor->setTempo(dlg.getTempo());
        m_adaptor->setResolution(dlg.getResolution());
        m_adaptor->setTickTime(dlg.isMusicalTime());
        m_adaptor->setRegAlsaMsg(dlg.isRegAlsaMsg());
        m_adaptor->setRegChannelMsg(dlg.isRegChannelMsg());
        m_adaptor->setRegCommonMsg(dlg.isRegCommonMsg());
        m_adaptor->setRegRealTimeMsg(dlg.isRegRealTimeMsg());
        m_adaptor->setRegSysexMsg(dlg.isRegSysexMsg());
        m_adaptor->setShowClientNames(dlg.showClientNames());
        m_adaptor->setTranslateSysex(dlg.translateSysex());
        m_adaptor->queue_set_tempo();
        m_adaptor->change_port_settings();
        setFixedFont(dlg.useFixedFont());
        setSortEvents(dlg.sortEvents());
        for (i = 0; i < 6; ++i) {
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
}

void KMidimon::toggleColumn(int colNum)
{
    m_view->setColumnHidden(colNum, !m_popupAction[colNum]->isChecked());
}

void KMidimon::toggleColumn0()
{
    toggleColumn(0);
}

void KMidimon::toggleColumn1()
{
    toggleColumn(1);
}

void KMidimon::toggleColumn2()
{
    toggleColumn(2);
}

void KMidimon::toggleColumn3()
{
    toggleColumn(3);
}

void KMidimon::toggleColumn4()
{
    toggleColumn(4);
}

void KMidimon::toggleColumn5()
{
    toggleColumn(5);
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

void KMidimon::setSortEvents(bool newValue)
{
    if (m_sortEvents != newValue) {
        m_sortEvents = newValue;
        m_model->setSorted(m_sortEvents);
        fileNew();
    }
}

void KMidimon::resizeColumns(const QModelIndex&, int, int)
{
    for( int i = 0; i < 6; ++i)
        m_view->resizeColumnToContents(i);
    if (m_sortEvents)
        m_view->scrollToBottom();
    else
        m_view->scrollToTop();
}
