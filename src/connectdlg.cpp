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

#include <QGroupBox>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QStringList>
#include <klocale.h>

#include "connectdlg.h"

ConnectDlg::ConnectDlg(QWidget *parent, const QStringList& clients,
        const QStringList& subs) :
    KDialog(parent)
{
    setCaption(i18n("Connections"));
    setModal(true);
    setButtons(Ok | Cancel);
    QWidget* w = mainWidget();
    QVBoxLayout* vbl1 = new QVBoxLayout(w);
    m_group = new QGroupBox(i18n("Available Ports"), w);
    vbl1->addWidget(m_group);
    QVBoxLayout* vbl2 = new QVBoxLayout(m_group);
    for (int i = 0; i < clients.size(); ++i) {
        QCheckBox *chk = new QCheckBox(clients[i], m_group);
        chk->setChecked(subs.contains(clients[i]) > 0);
        vbl2->addWidget(chk);
    }
}

QStringList ConnectDlg::getSelected() const
{
    QStringList lst;
    QList<QCheckBox*> checks = m_group->findChildren<QCheckBox*> ();
    foreach ( QCheckBox* chk, checks ) {
        if (chk->isChecked()) {
            lst += chk->text().remove("&");
        }
    }
    return lst;
}
