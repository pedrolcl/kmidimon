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

#ifndef QT_KDE_H_
#define QT_KDE_H_

// Standard C++ library
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

// QT3 headers

// Common headers, or used by sources generated by moc
#include <qmap.h> 
#include <qglobal.h>
#include <private/qucomextra_p.h> 
#include <qmetaobject.h>
#include <qobjectdefs.h>
#include <qsignalslotimp.h>
#include <qstyle.h>

// Headers used by KDE3
#include <q3accel.h>
#include <qapplication.h>
#include <qbitmap.h>
#include <qbrush.h>
#include <qbuffer.h>
#include <q3buttongroup.h>
#include <qbutton.h>
#include <q3canvas.h>
#include <qcheckbox.h>
#include <qcolor.h>
#include <qcombobox.h>
#include <q3cstring.h>
#include <qcursor.h>
#include <qdatastream.h>
#include <qdatetime.h>
#include <qdialog.h>
#include <q3dict.h>
#include <qdir.h>
#include <qdom.h>
#include <q3dragobject.h>
#include <qdrawutil.h>
#include <qevent.h>
#include <qeventloop.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qfont.h>
#include <qfontinfo.h>
#include <qfontmetrics.h>
#include <q3frame.h>
#include <q3garray.h>
#include <q3grid.h>
#include <q3groupbox.h>
#include <qpointer.h>
#include <q3hbox.h>
#include <q3header.h>
#include <qicon.h>
#include <qimage.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <q3listbox.h>
#include <q3listview.h>
#include <q3memarray.h>
#include <qmutex.h>
#include <qobject.h>
#include <qobject.h>
#include <q3paintdevicemetrics.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpen.h>
#include <qpixmap.h>
#include <q3pointarray.h>
#include <qpoint.h>
#include <q3popupmenu.h>
#include <qprinter.h>
#include <q3progressdialog.h>
#include <q3ptrdict.h>
#include <q3ptrlist.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qrect.h>
#include <qregexp.h>
#include <qregion.h>
#include <qscrollbar.h>
#include <q3scrollview.h>
#include <qsessionmanager.h>
#include <qsignalmapper.h>
#include <qsize.h>
#include <qsizepolicy.h>
#include <qslider.h>
#include <qsocketnotifier.h>
#include <qspinbox.h>
#include <qsplitter.h>
#include <qstring.h>
#include <qstringlist.h>
#include <q3strlist.h>
#include <q3table.h>
#include <qtabwidget.h>
#include <qtextcodec.h>
#include <q3textedit.h>
#include <q3textstream.h>
#include <qthread.h>
#include <qtimer.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <qvalidator.h>
#include <q3valuelist.h>
#include <q3valuevector.h>
#include <qvariant.h>
#include <q3vbox.h>
#include <q3vgroupbox.h>
#include <q3whatsthis.h>
#include <qwidget.h>
#include <q3widgetstack.h>
#include <qmatrix.h>
#include <qxml.h>

// KDE3 headers
#include <kaboutdata.h>
#include <kaccel.h>
#include <kactioncollection.h>
#include <kaction.h>
#include <kapplication.h>
#include <kapplication.h>
#include <karrowbutton.h>
#include <kcmdlineargs.h>
#include <kcolordialog.h>
#include <kcombobox.h>
#include <k3command.h>
#include <kcompletion.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kdcopactionproxy.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kdialogbase.h>
#include <kdialog.h>
#include <kdiskfreespace.h>
#include <k3dockwidget.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kfile.h>
#include <kfilterdev.h>
#include <kfontrequester.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <kio/netaccess.h>
#include <kkeydialog.h>
#include <kled.h>
#include <klineedit.h>
#include <k3listview.h>
#include <klocale.h>
#include <kmainwindow.h>
#include <kmessagebox.h>
#include <kmimetype.h>

#include <kmenu.h>
#include <kprinter.h>
#include <k3process.h>
#include <kprogressdialog.h>
#include <kpushbutton.h>
#include <ksqueezedtextlabel.h>
#include <kstandarddirs.h>
#include <kstatusbar.h>
#include <kstdaccel.h>
#include <kstandardaction.h>
#include <kstandarddirs.h>
#include <ktabwidget.h>
#include <ktemporaryfile.h>
#include <ktip.h>
#include <ktoolbar.h>
#include <kuniqueapplication.h>
#include <kurl.h>
#include <kxmlguiclient.h>
#include <kxmlguifactory.h>

#endif /*QT_KDE_H_*/
