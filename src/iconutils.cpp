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

#include <QApplication>
#include <QPainter>
#include "iconutils.h"

namespace IconUtils
{
    void PaintPixmap(QPixmap &pixmap, const QColor& color)
    {
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), color);
    }

    QPixmap GetPixmap(QWidget* widget, const QString& fileName)
    {
        QPixmap pixmap(fileName);
        QColor color = widget->palette().color(QPalette::Active, QPalette::Foreground);
        PaintPixmap(pixmap, color);
        return pixmap;
    }

    void SetLabelIcon(QLabel *label, const QString& fileName)
    {
        label->setPixmap(GetPixmap(label, fileName));
    }

    void SetupComboFigures(QComboBox *combo)
    {
        QList<QPair<QString,QString>> elements = {
            {QApplication::tr("Whole"),         ":/icons/1.png" },
            {QApplication::tr("Half"),          ":/icons/2.png" },
            {QApplication::tr("Quarter"),       ":/icons/4.png" },
            {QApplication::tr("Eight"),         ":/icons/8.png" },
            {QApplication::tr("Sixteenth"),     ":/icons/16.png" },
            {QApplication::tr("Thirty-Second"), ":/icons/32.png" },
            {QApplication::tr("Sixty-Fourth"),  ":/icons/64.png" }
        };
        for(auto p : elements) {
            combo->addItem(QIcon(GetPixmap(combo, p.second)), p.first);
        }
    }

    void SetWindowIcon(QWidget *widget)
    {
        widget->setWindowIcon(QIcon(GetPixmap(widget, ":/icons/midi/icon32.png")));
    }
}
