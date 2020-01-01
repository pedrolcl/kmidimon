/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2020 Pedro Lopez-Cabanillas                        *
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

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFileInfo>
#include <QDebug>

#include "kmidimon.h"

#define LITERAL(s) #s
#define STRINGIFY(s) LITERAL(s)

const QString QSTR_DOMAIN("kmidimon.sourceforge.net");
const QString QSTR_APPNAME("Drumstick MIDI Monitor");
const QString QSTR_DESCRIPTION("ALSA Sequencer based MIDI Monitor");
const QString QSTR_VERSION(STRINGIFY(VERSION));

int main (int argc, char **argv)
{
    QCoreApplication::setOrganizationName(QSTR_DOMAIN);
    QCoreApplication::setOrganizationDomain(QSTR_DOMAIN);
    QCoreApplication::setApplicationName(QSTR_APPNAME);
    QCoreApplication::setApplicationVersion(QSTR_VERSION);
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QSTR_DESCRIPTION);
    auto helpOption = parser.addHelpOption();
    auto versionOption = parser.addVersionOption();
    parser.addPositionalArgument("file", "Input SMF/KAR/OVE/WRK file name.", "file");
    parser.process(app);

    if (parser.isSet(versionOption) || parser.isSet(helpOption)) {
        return 0;
    }

    QStringList fileNames, positionalArgs = parser.positionalArguments();
    for(const QString& a : positionalArgs) {
        QFileInfo f(a);
        if (f.exists())
            fileNames += f.canonicalFilePath();
        else
            qWarning() << "File not found: " << a << endl;
    }

    KMidimon mainWin;
    if (!fileNames.isEmpty()) {
        mainWin.open(fileNames.first());
    }
    mainWin.show();
    return app.exec();
}
