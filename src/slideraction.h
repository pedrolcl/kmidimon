/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2010 Pedro Lopez-Cabanillas                        *
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
/***************************************************************************
 *   Other copyright notices for this file include:                        *                                              *
 *   copyright (C) 2003      kiriuja  <kplayer-dev@en-directo.net>         *
 *   copyright (C) 2003-2010                                               *
 *   Umbrello UML Modeller Authors <uml-devel@uml.sf.net>                  *
 ***************************************************************************/

#ifndef SLIDERACTION_H
#define SLIDERACTION_H

#include <kaction.h>

#include <QtGui/QSlider>
#include <QtGui/QFrame>

class QKeyEvent;

/**
 * KPlayer's slider widget.
 * Taken from umbrello (kdesdk SVN 992814, 2009-07-08) by Pedro Lopez-Cabanillas
 * (with small changes)
 *
 * Taken from kplayer CVS 2003-09-21 (kplayer > 0.3.1) by Jonathan Riddell
 * @author kiriuja
 */
class KPlayerSlider : public QSlider
{
    Q_OBJECT

public:
    explicit KPlayerSlider (Qt::Orientation, QWidget* parent = 0);
    virtual ~KPlayerSlider() {}

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;
    void setPageStep (int);
    void setup (int minimum, int maximum, int value, int pageStep, int lineStep = 1);

protected:
    friend class KPlayerSliderAction;
    friend class KPlayerPopupSliderAction;
};

/**
 * KPlayer popup frame.
 * @author kiriuja
 */
class KPlayerPopupFrame : public QFrame
{
    Q_OBJECT

public:
    KPlayerPopupFrame (QWidget* parent = 0);
    virtual ~KPlayerPopupFrame();

protected:
    virtual void keyPressEvent (QKeyEvent*);
};

/**
 * Action representing a popup slider activated by a toolbar button.
 * @author kiriuja
 */
class KPlayerPopupSliderAction : public KAction
{
    Q_OBJECT

public:
    KPlayerPopupSliderAction (const QObject* receiver, const char* slot, QObject *parent);
    virtual ~KPlayerPopupSliderAction();

    /** Returns a pointer to the KPlayerSlider object. */
    KPlayerSlider* slider() { return m_slider; }

protected slots:
    virtual void slotTriggered();

protected:
    KPlayerSlider*      m_slider;  ///< The slider.
    KPlayerPopupFrame*  m_frame;   ///< The popup frame.
};

#endif
