/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005 by Pedro Lopez-Cabanillas                          *
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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/*
 * Copyright (C) 2005 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
 */

#ifndef _KMIDIMONWIDGET_H_
#define _KMIDIMONWIDGET_H_

#include "kmidimonwidgetbase.h"
#include "sequencerclient.h"

class KMidimonWidget : public KMidimonWidgetBase
{
    Q_OBJECT

public:
    KMidimonWidget(QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~KMidimonWidget();
    /*$PUBLIC_FUNCTIONS$*/
    void clear();
    void add(MidiEvent *ev);
    void saveTo(QString path);
    
public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/

};

#endif

