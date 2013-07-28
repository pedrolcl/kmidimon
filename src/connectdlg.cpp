/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2013 Pedro Lopez-Cabanillas                        *
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

#include "connectdlg.h"

#include <QGroupBox>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QStringList>
#include <QLabel>
#include <klocale.h>
#include <kcombobox.h>

ConnectDlg::ConnectDlg( QWidget *parent,
                        const QStringList& inputs,
                        const QStringList& subs,
                        const QStringList& outputs,
                        const QString& out ) :
    KDialog(parent)
{
    setCaption(i18n("Connections"));
    setModal(true);
    setButtons(Ok | Cancel);
    QWidget* w = mainWidget();
    w->setMinimumWidth(320);
    QVBoxLayout* vbl1 = new QVBoxLayout(w);
    m_group = new QGroupBox(i18n("Available Input Connections:"), w);
    vbl1->addWidget(m_group);
    QVBoxLayout* vbl2 = new QVBoxLayout(m_group);
    for (int i = 0; i < inputs.size(); ++i) {
        QCheckBox *chk = new QCheckBox(inputs[i], m_group);
        chk->setChecked(subs.contains(inputs[i]) > 0);
        vbl2->addWidget(chk);
    }
    QLabel* lbl = new QLabel(i18n("<b>Output Connection:</b>"), w);
    vbl1->addWidget(lbl);
    m_output = new KComboBox(w);
    m_output->addItems(outputs);
    m_output->setCurrentIndex(-1);
    for (int i = 0; i < m_output->count(); ++i) {
        if (m_output->itemText(i) == out) {
            m_output->setCurrentIndex(i);
            break;
        }
    }
    vbl1->addWidget(m_output);
}

QStringList ConnectDlg::getSelectedInputs() const
{
    QStringList lst;
    QList<QCheckBox*> checks = m_group->findChildren<QCheckBox*> ();
    foreach ( QCheckBox* chk, checks ) {
        if (chk->isChecked()) {
            lst += chk->text().remove('&');
        }
    }
    return lst;
}

QString ConnectDlg::getSelectedOutput() const
{
    return m_output->currentText();
}
