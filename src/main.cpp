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

#include <exception>
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kmessagebox.h>
#include "kmidimon.h"

static const char description[] =
    I18N_NOOP("KDE MIDI monitor using ALSA sequencer");

static const char version[] = VERSION;

/*static KCmdLineOptions options[] =
{
//    { "+[URL]", I18N_NOOP( "Document to open." ), 0 },
    KCmdLineLastOption
};*/

int main(int argc, char **argv)
{
	KAboutData about("kmidimon", I18N_NOOP("KMidimon"), version, description,
			KAboutData::License_GPL, "(C) 2005-2008 Pedro Lopez-Cabanillas", 
			0, 0, "plcl@users.sourceforge.net");
	about.addAuthor("Pedro Lopez-Cabanillas", 0, "plcl@users.sourceforge.net");
	about.addCredit("Christoph Eckert", 
			I18N_NOOP("Documentation, good ideas and suggestions"));
	KCmdLineArgs::init(argc, argv, &about);
	//KCmdLineArgs::addCmdLineOptions( options );
	KApplication app;
	KMidimon *mainWin = 0;

	try {
		if (app.isRestored())
		{
			RESTORE(KMidimon);
		}
		else
		{
			// no session.. just start up normally
			//KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
			/// @todo do something with the command line args here
			mainWin = new KMidimon();
			app.setMainWidget( mainWin );
			mainWin->show();
			//args->clear();
		}
		// mainWin has WDestructiveClose flag by default, so it will delete itself.
		return app.exec();
	} catch ( std::exception *ex ) {
		KMessageBox::error(0, ex->what());
	}
}
