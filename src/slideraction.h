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
/***************************************************************************
 *   Other copyright notices for this file include:                        *                                              *
 *   copyright (C) 2003      kiriuja  <kplayer-dev@en-directo.net>         *
 *   copyright (C) 2003-2024                                               *
 *   Umbrello UML Modeller Authors <uml-devel@uml.sf.net>                  *
 ***************************************************************************/

#ifndef SLIDERACTION_H
#define SLIDERACTION_H

#include <QAction>
#include <QSlider>
#include <QFrame>

class QKeyEvent;

/**
 * KPlayer's slider widget.
 * Taken from umbrello (kdesdk SVN 992814, 2009-07-08) by Pedro Lopez-Cabanillas
 * (with small changes)
 *
 * Taken from kplayer CVS 2003-09-21 (kplayer > 0.3.1) by Jonathan Riddell
 * @author kiriuja
 */
class PlayerSlider : public QSlider
{
    Q_OBJECT

public:
    explicit PlayerSlider (Qt::Orientation, QWidget* parent = nullptr);
    virtual ~PlayerSlider() {}

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
    void setPageStep (int);
    void setup (int minimum, int maximum, int value, int pageStep, int lineStep = 1);

protected:
    friend class PlayerSliderAction;
    friend class PlayerPopupSliderAction;
};

/**
 * KPlayer popup frame.
 * @author kiriuja
 */
class PlayerPopupFrame : public QFrame
{
    Q_OBJECT

public:
    PlayerPopupFrame (QWidget* parent = nullptr);
    virtual ~PlayerPopupFrame();

protected:
    virtual void keyPressEvent (QKeyEvent*) override;
};

/**
 * Action representing a popup slider activated by a toolbar button.
 * @author kiriuja
 */
class PlayerPopupSliderAction : public QAction
{
    Q_OBJECT

public:
    PlayerPopupSliderAction (const QObject* receiver, const char* slot, QObject *parent);
    virtual ~PlayerPopupSliderAction();

    /** Returns a pointer to the PlayerSlider object. */
    PlayerSlider* slider() { return m_slider; }

protected slots:
    virtual void slotTriggered();

protected:
    PlayerSlider*      m_slider;  ///< The slider.
    PlayerPopupFrame*  m_frame;   ///< The popup frame.
};

#endif
