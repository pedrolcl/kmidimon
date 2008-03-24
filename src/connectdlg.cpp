/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2008 Pedro Lopez-Cabanillas                        *
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

#include <klocale.h>
#include <qvbox.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qstringlist.h>
#include <qobjectlist.h>
#include "connectdlg.h"
#include "debugdef.h"

ConnectDlg::ConnectDlg( QWidget *parent, 
			const QStringList& clients, 
			const QStringList& subs ):
	KDialogBase(parent, NULL, true, i18n("Connections"), Ok | Cancel )
{
    QVBox *vbox = makeVBoxMainWidget();
    group = new QGroupBox(1, Qt::Horizontal, i18n("Available Ports"), vbox);
    for ( unsigned int i = 0; i < clients.size(); ++i) {
	QCheckBox *chk = new QCheckBox(clients[i], group);
	chk->setChecked(subs.contains(clients[i]) > 0);
    }
}

QStringList ConnectDlg::getSelected() const
{
    QStringList lst;
    QObjectList btns = *group->children();
    for (QObject *obj = btns.first(); obj; obj = btns.next()) {
    	if (obj->isA("QCheckBox")) {
    	    QCheckBox *chk = (QCheckBox *)obj;
    	    if (chk->isChecked()) {
                //DEBUGSTREAM << "checked: " << chk->text() << endl;
		lst += chk->text().remove("&");
            }
    	}
    }
    return lst;
}

#include "connectdlg.moc"
