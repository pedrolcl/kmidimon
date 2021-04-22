/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2021 Pedro Lopez-Cabanillas                        *
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

#include <QTextCodec>
#include <QStyle>
#include <QStyleFactory>
#include "configdialog.h"
#include "iconutils.h"

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    setWindowIcon(IconUtils::GetIcon("midi/icon64", true));
    setWindowTitle(tr("KMidimon Configuration", "@title:window"));
    initEncodings();
    initStyles();
}

bool ConfigDialog::showColumn(int colNum)
{
    switch (colNum) {
    case 0:
        return ui.m_showTicksColumn->isChecked();
    case 1:
        return ui.m_showTimeColumn->isChecked();
    case 2:
        return ui.m_showSourceColumn->isChecked();
    case 3:
        return ui.m_showEventTypeColumn->isChecked();
    case 4:
        return ui.m_showChannelColumn->isChecked();
    case 5:
        return ui.m_showData1Column->isChecked();
    case 6:
        return ui.m_showData2Column->isChecked();
    case 7:
        return ui.m_showData3Column->isChecked();
    }
    return false;
}

void ConfigDialog::setShowColumn(int colNum, bool newValue)
{
    switch (colNum) {
    case 0:
        ui.m_showTicksColumn->setChecked(newValue);
        break;
    case 1:
        ui.m_showTimeColumn->setChecked(newValue);
        break;
    case 2:
        ui.m_showSourceColumn->setChecked(newValue);
        break;
    case 3:
        ui.m_showEventTypeColumn->setChecked(newValue);
        break;
    case 4:
        ui.m_showChannelColumn->setChecked(newValue);
        break;
    case 5:
        ui.m_showData1Column->setChecked(newValue);
        break;
    case 6:
        ui.m_showData2Column->setChecked(newValue);
        break;
    case 7:
        ui.m_showData3Column->setChecked(newValue);
        break;
    }
}

void ConfigDialog::setInstrumentName( const QString& name )
{
    int index = ui.m_instruments->findText( name );
    ui.m_instruments->setCurrentIndex( index );
}

void ConfigDialog::setInstruments( const QStringList& items )
{
    ui.m_instruments->clear();
    ui.m_instruments->addItems(items);
}

void ConfigDialog::initEncodings()
{
    ui.m_codecs->clear();
    ui.m_codecs->addItem(tr("Default ( ASCII )", "@item:inlistbox Default MIDI text encoding"));
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
    ui.m_codecs->addItems(encodings);
}

QString ConfigDialog::getEncoding()
{
    return ui.m_codecs->currentText();
}

void ConfigDialog::setEncoding(const QString& name)
{
    int index = ui.m_codecs->findText( name );
    ui.m_codecs->setCurrentIndex( index );
}

void ConfigDialog::initStyles()
{
    QStringList styleNames = QStyleFactory::keys();
    ui.m_styles->addItems(styleNames);
    QString currentStyle = qApp->style()->objectName();
    foreach(const QString& s, styleNames) {
        if (QString::compare(s, currentStyle, Qt::CaseInsensitive) == 0) {
            ui.m_styles->setCurrentText(s);
            break;
        }
    }
}

QString ConfigDialog::getStyle()
{
    return ui.m_styles->currentText();
}

void ConfigDialog::setStyle(const QString& name)
{
    int idx = ui.m_styles->findText(name);
    ui.m_styles->setCurrentIndex(idx);
}

bool ConfigDialog::getDarkMode()
{
    return ui.m_forcedDarkMode->isChecked();
}

void ConfigDialog::setDarkMode(bool dark)
{
    ui.m_forcedDarkMode->setChecked(dark);
}

bool ConfigDialog::getInternalIcons()
{
    return ui.m_forcedTheme->isChecked();
}

void ConfigDialog::setInternalIcons(bool internal)
{
    ui.m_forcedTheme->setChecked(internal);
}
