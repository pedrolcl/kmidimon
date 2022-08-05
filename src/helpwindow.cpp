/*
    Drumstick MIDI monitor based on the ALSA Sequencer
    Copyright (C) 2006-2022, Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <QDebug>
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QToolBar>
#include <QShowEvent>
#include <QCloseEvent>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#if QT_VERSION < QT_VERSION_CHECK(5,14,0)
#include <QDesktopWidget>
#else
#include <QScreen>
#endif

#include "kmidimon.h"
#include "helpwindow.h"
#include "iconutils.h"

HelpWindow::HelpWindow(QWidget *parent): QMainWindow(parent)
{
    setObjectName(QString::fromUtf8("HelpWindow"));
    IconUtils::SetWindowIcon(this);
    setWindowFlag(Qt::Tool, true);
    setAttribute(Qt::WA_DeleteOnClose, false);

    QToolBar* tbar = new QToolBar(this);
    tbar->setObjectName("toolbar");
    tbar->setMovable(false);
    tbar->setFloatable(false);
    tbar->setIconSize(QSize(22,22));
    tbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addToolBar(tbar);

    m_textBrowser = new QTextBrowser(this);
    m_home = new QAction(tr("&Home"), this);
    m_back = new QAction(tr("&Back"), this);
    m_close = new QAction(tr("Close"), this);
    m_zoomIn = new QAction(tr("Zoom In"), this);
    m_zoomOut = new QAction(tr("Zoom Out"), this);
    QWidget* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    tbar->addAction(m_home);
    tbar->addAction(m_back);
    tbar->addAction(m_zoomIn);
    tbar->addAction(m_zoomOut);
    tbar->addWidget(spacer);
    tbar->addAction(m_close);

    setCentralWidget(m_textBrowser);

    connect(m_home, &QAction::triggered, m_textBrowser, &QTextBrowser::home);
    connect(m_back, &QAction::triggered, m_textBrowser, &QTextBrowser::backward);
    connect(m_zoomIn, &QAction::triggered, this, [=]{ m_textBrowser->zoomIn(); });
    connect(m_zoomOut, &QAction::triggered, this, [=]{ m_textBrowser->zoomOut(); });
    connect(m_close, &QAction::triggered, this, &QWidget::close);
    connect(m_textBrowser, &QTextBrowser::sourceChanged, this, &HelpWindow::updateWindowTitle);
    connect(m_textBrowser, &QTextBrowser::backwardAvailable, m_back, &QAction::setEnabled);

    m_textBrowser->setOpenExternalLinks(true);
    m_textBrowser->setSearchPaths({":/help/en",":/help", ":/"});

    applySettings();
    retranslateUi();
}

void HelpWindow::updateWindowTitle()
{
    setWindowTitle(m_textBrowser->documentTitle());
}

void HelpWindow::setIcons(bool internal)
{
    m_internalIcons = internal;
}

void HelpWindow::readSettings()
{
    QSettings settings;
    settings.beginGroup("HelpWindow");
    const QByteArray geometry = settings.value("geometry").toByteArray();
    const QByteArray state = settings.value("windowState").toByteArray();
    const int fontSize = settings.value("fontSize", 0).toInt();

    if (geometry.isEmpty()) {
        const QRect availableGeometry =
#if QT_VERSION < QT_VERSION_CHECK(5,14,0)
                QApplication::desktop()->availableGeometry(this);
#else
                screen()->availableGeometry();
#endif
        QSize size(640,480);
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
                                        size, availableGeometry));
    } else {
        restoreGeometry(geometry);
    }
    if (!state.isEmpty()) {
        restoreState(state);
    }
    if (fontSize > 0) {
        //qDebug() << Q_FUNC_INFO << fontSize;
        auto font = m_textBrowser->font();
        font.setPointSize(fontSize);
        m_textBrowser->setFont(font);
    }
    settings.endGroup();
}

void HelpWindow::writeSettings()
{
    QSettings settings;
    settings.beginGroup("HelpWindow");
    auto fontSize = m_textBrowser->font().pointSize();
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("fontSize", fontSize);
    settings.endGroup();
    //qDebug() << Q_FUNC_INFO << fontSize;
}

void HelpWindow::showEvent(QShowEvent *event)
{
    static bool firstTime = true;
    QMainWindow::showEvent(event);
    if (firstTime) {
        readSettings();
        firstTime = false;
    }
}

void HelpWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

void HelpWindow::retranslateUi()
{
    QString language = qobject_cast<KMidimon*>(parent())->configuredLanguage();
    if (language == "C") {
        language = "en";
    }
    QString page = QStringLiteral("help/%1/index.html").arg(language);
    QDir hdir(":/");
    QFileInfo finfo(hdir, page);
    if (!finfo.exists()) {
        page = "help/en/index.html";
    }
    m_textBrowser->clear();
    m_textBrowser->setSource(page);
    m_textBrowser->clearHistory();
    updateWindowTitle();
    m_home->setText(tr("&Home"));
    m_back->setText(tr("&Back"));
    m_close->setText(tr("Close"));
    m_zoomIn->setText(tr("Zoom In"));
    m_zoomOut->setText(tr("Zoom Out"));
}

void HelpWindow::applySettings()
{
    m_home->setIcon(IconUtils::GetIcon("go-home"));
    m_back->setIcon(IconUtils::GetIcon("go-previous"));
    m_close->setIcon(IconUtils::GetIcon("window-close"));
    m_zoomIn->setIcon(IconUtils::GetIcon("format-font-size-more"));
    m_zoomOut->setIcon(IconUtils::GetIcon("format-font-size-less"));
}
