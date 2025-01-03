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

#include <QGroupBox>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QStringList>
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>
#include "connectdlg.h"
#include "iconutils.h"

ConnectDlg::ConnectDlg( QWidget *parent,
                        const QStringList& inputs,
                        const QStringList& subs,
                        const QStringList& outputs,
                        const QString& out ) :
    QDialog(parent)
{
    setWindowTitle(tr("Connections"));
    IconUtils::SetWindowIcon(this);

    setModal(true);
    setMinimumWidth(320);
    QVBoxLayout* vbl1 = new QVBoxLayout(this);
    m_group = new QGroupBox(tr("Available Input Connections:"), this);
    vbl1->addWidget(m_group);
    QVBoxLayout* vbl2 = new QVBoxLayout(m_group);
    for (int i = 0; i < inputs.size(); ++i) {
        QCheckBox *chk = new QCheckBox(inputs[i], m_group);
        chk->setChecked(subs.contains(inputs[i]) > 0);
        vbl2->addWidget(chk);
    }
    m_thru = new QCheckBox(tr("MIDI Thru on MIDI OUT"), this);
    vbl1->addWidget(m_thru);
    QLabel* lbl = new QLabel(tr("<b>Output Connection:</b>"), this);
    vbl1->addWidget(lbl);
    m_output = new QComboBox(this);
    m_output->addItems(outputs);
    m_output->setCurrentIndex(-1);
    for (int i = 0; i < m_output->count(); ++i) {
        if (m_output->itemText(i) == out) {
            m_output->setCurrentIndex(i);
            break;
        }
    }
    vbl1->addWidget(m_output);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    vbl1->addWidget(buttonBox);
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

void ConnectDlg::setThruEnabled(bool enable)
{
    m_thru->setChecked(enable);
}

bool ConnectDlg::isThruEnabled() const
{
    return m_thru->isChecked();
}
