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

#include <exception>
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "kmidimon.h"
#include "aseqmmcommon.h"

using namespace aseqmm;

static const char description[] =
    I18N_NOOP("KDE MIDI monitor using ALSA sequencer");

static const char version[] = VERSION;

int main(int argc, char **argv)
{
	KAboutData about("kmidimon", 0, ki18n("KMidimon"), version,
	                 ki18n(description), KAboutData::License_GPL,
	                 ki18n("(C) 2005-2009 Pedro Lopez-Cabanillas"),
	                 KLocalizedString(),
	                 "http://kmetronome.sourceforge.net/kmidimon",
	                 "plcl@users.sourceforge.net");
	about.addAuthor(ki18n("Pedro Lopez-Cabanillas"), KLocalizedString(),
	                "plcl@users.sourceforge.net");
	about.addCredit(ki18n("Christoph Eckert"),
	                ki18n("Documentation, good ideas and suggestions"));
	KCmdLineArgs::init(argc, argv, &about);
    //KCmdLineOptions options;
    //options.add("+[URL]", ki18n( "Document to open" ));
	//KCmdLineArgs::addCmdLineOptions( options );
	KApplication app;
	KMidimon *mainWin = 0;
    if (app.isSessionRestored()) {
        RESTORE(KMidimon);
    } else {
        // no session.. just start up normally
        //KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
        mainWin = new KMidimon;
        mainWin->show();
        //args->clear();
    }
    return app.exec();
}
