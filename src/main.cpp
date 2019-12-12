/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2019 Pedro Lopez-Cabanillas                        *
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

#include "kmidimon.h"
#include <QApplication>
#include <QLocale>

//static const char description[] =
//        I18N_NOOP("KDE MIDI monitor using ALSA sequencer");

//static const char version[] = VERSION;

const QString QSTR_DOMAIN("kmidimon.sourceforge.net");
const QString QSTR_APPNAME("kmidimon");

int main (int argc, char **argv)
{
//    KAboutData about("kmidimon", 0, ki18n("KMidimon"), version, ki18n(
//            description), KAboutData::License_GPL, ki18n(
//            "(C) 2005-2011 Pedro Lopez-Cabanillas"), KLocalizedString(),
//            "http://kmidimon.sourceforge.net", "plcl@users.sourceforge.net");
//    about.addAuthor(ki18n("Pedro Lopez-Cabanillas"), KLocalizedString(),
//            "plcl@users.sourceforge.net");
//    about.addCredit(ki18n("Christoph Eckert"), ki18n(
//            "Documentation, good ideas and suggestions"));
    QCoreApplication::setOrganizationName(QSTR_DOMAIN);
    QCoreApplication::setOrganizationDomain(QSTR_DOMAIN);
    QCoreApplication::setApplicationName(QSTR_APPNAME);
    QApplication app(argc, argv);
    KMidimon *mainWin = new KMidimon;
//    if (args->count() > 0)
//        mainWin->openURL(args->url(0));
    mainWin->show();
    return app.exec();
}
