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
/***************************************************************************
 *   Other copyright notices for this file include:                        *                                              *
 *   copyright (C) 2003      kiriuja  <kplayer-dev@en-directo.net>         *
 *   copyright (C) 2003-2019                                               *
 *   Umbrello UML Modeller Authors <uml-devel@uml.sf.net>                  *
 ***************************************************************************/

/*
 * Taken from umbrello (kdesdk SVN 992814, 2009-07-08) by Pedro Lopez-Cabanillas
 * (with small changes)
 *
 * Taken from kplayer CVS 2003-09-21 (kplayer > 0.3.1) by Jonathan Riddell.
 */

#include "slideraction.h"

#include <QToolBar>
#include <QKeyEvent>
#include <QDesktopWidget>
#include <QApplication>
#include <QWidgetAction>

/**
 * The PlayerPopupFrame constructor. Parameters are passed on to QFrame.
 */
PlayerPopupFrame::PlayerPopupFrame (QWidget* parent)
  : QFrame (parent, Qt::Popup)
{
    setFrameStyle(QFrame::Raised | QFrame::Panel);
    setLineWidth(2);
}

/**
 * The PlayerPopupFrame destructor. Does nothing.
 */
PlayerPopupFrame::~PlayerPopupFrame()
{
}

/**
 * Closes the popup frame when Alt, Tab, Esc, Enter or Return is pressed.
 */
void PlayerPopupFrame::keyPressEvent (QKeyEvent* ev)
{
    switch ( ev->key() ) {
    case Qt::Key_Alt:
    case Qt::Key_Tab:
    case Qt::Key_Escape:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        close();
    }
}

/**
 * The PlayerPopupSliderAction constructor. Parameters are passed on to KAction.
 */
PlayerPopupSliderAction::PlayerPopupSliderAction (const QObject* receiver, const char* slot,
                                                    QObject *parent)
  : QAction(parent)
{
    m_frame = new PlayerPopupFrame;
    m_slider = new PlayerSlider(Qt::Vertical, m_frame);
    m_frame->resize (36, m_slider->sizeHint().height() + 4);
    m_slider->setGeometry(m_frame->contentsRect());
    connect (this, SIGNAL(triggered()), this , SLOT(slotTriggered()));
    connect (m_slider, SIGNAL(valueChanged(int)), receiver, slot);
}

/**
 * The PlayerPopupSliderAction destructor. Deletes the PlayerPopupFrame.
 */
PlayerPopupSliderAction::~PlayerPopupSliderAction()
{
    delete m_frame;
    m_frame = 0;
}

/**
 * Pops up the slider.
 */
void PlayerPopupSliderAction::slotTriggered()
{
    QPoint point;

    QList<QWidget*> associatedWidgetsList = associatedWidgets();

    QWidget* associatedWidget = 0;
    QWidget* associatedToolButton = 0;

    // find the toolbutton which was clicked on
    foreach(associatedWidget, associatedWidgetsList) {
      if (QToolBar* associatedToolBar = dynamic_cast<QToolBar*>(associatedWidget)) {
        associatedToolButton = associatedToolBar->childAt(associatedToolBar->mapFromGlobal(QCursor::pos()));
        if(associatedToolButton) {
          break;  // found the tool button which was clicked
        }
      }
    }

    if ( associatedToolButton  ) {
      point = associatedToolButton->mapToGlobal(
              QPoint( associatedToolButton->width() / 2 - m_frame->width() / 2,
                      associatedToolButton->height() ));

    } else {

      point = QCursor::pos() - QPoint (m_frame->width() / 2, m_frame->height() / 2);
      if ( point.x() + m_frame->width() > QApplication::desktop()->width() )
        point.setX (QApplication::desktop()->width() - m_frame->width());
      if ( point.y() + m_frame->height() > QApplication::desktop()->height() )
        point.setY (QApplication::desktop()->height() - m_frame->height());
      if ( point.x() < 0 )
        point.setX (0);
      if ( point.y() < 0 )
        point.setY (0);
     }

    // qDebug() << "Point: " << point.x() << "x" << point.y() << "\n";
    m_frame->move (point);
    m_frame->show();
    m_slider->setFocus();
}

/**
 * The PlayerSlider constructor. Parameters are passed on to QSlider.
 */
PlayerSlider::PlayerSlider (Qt::Orientation orientation, QWidget* parent)
  : QSlider (orientation, parent)
{
    setup(0, 200, 100, 10);
    setTickPosition (QSlider::TicksBothSides);
}

/**
 * The size hint.
 */
QSize PlayerSlider::sizeHint() const
{
    QSize hint = QSlider::sizeHint();
    int length = 200;
    if ( orientation() == Qt::Horizontal )
    {
        if ( hint.width() < length )
            hint.setWidth (length);
    }
    else
    {
        if ( hint.height() < length )
            hint.setHeight (length);
    }
    return hint;
}

/**
 * The minimum size hint.
 */
QSize PlayerSlider::minimumSizeHint() const
{
    // uDebug() << "PlayerSlider minimum size hint\n";
    QSize hint = QSlider::minimumSizeHint();
    int length = 200;
    if ( orientation() == Qt::Horizontal )
    {
        if ( hint.width() < length )
            hint.setWidth (length);
    }
    else
    {
        if ( hint.height() < length )
            hint.setHeight (length);
    }
    return hint;
}

/**
 * Sets the page step.
 */
void PlayerSlider::setPageStep (int pageStep)
{
    QSlider::setPageStep (pageStep);
    setTickInterval (pageStep);
}

/**
 * Sets up the slider by setting five options in one go.
 */
void PlayerSlider::setup (int minimum, int maximum, int value, int pageStep, int singleStep)
{
    setMinimum (minimum);
    setMaximum (maximum);
    setSingleStep (singleStep);
    setPageStep (pageStep);
    setValue (value);
}
