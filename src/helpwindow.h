/*
    Drumstick MIDI monitor based on the ALSA Sequencer
    Copyright (C) 2006-2023, Pedro Lopez-Cabanillas <plcl@users.sf.net>

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

#ifndef HELPWINDOW_H
#define HELPWINDOW_H

#include <QObject>
#include <QMainWindow>
#include <QTextBrowser>
#include <QAction>
#include <QCloseEvent>
#include <QShowEvent>

class HelpWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit HelpWindow(QWidget *parent = nullptr);
    void readSettings();
    void writeSettings();
    void retranslateUi();
    void applySettings();
    void setIcons(bool internal);

private slots:
    void updateWindowTitle();
    void showEvent( QShowEvent *event ) override;
    void closeEvent( QCloseEvent *event ) override;

private:
    QTextBrowser *m_textBrowser;
    QAction *m_home;
    QAction *m_back;
    QAction *m_close;
    QAction *m_zoomIn;
    QAction *m_zoomOut;
    bool m_internalIcons;
};

#endif // HELPWINDOW_H
