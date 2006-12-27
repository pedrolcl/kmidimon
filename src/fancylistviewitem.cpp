/***************************************************************************
 *   KMidimon - ALSA sequencer based MIDI monitor                          *
 *   Copyright (C) 2005-2006 Pedro Lopez-Cabanillas                        *
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

#include <qpainter.h>

#include "fancylistviewitem.h"

void FancyListViewItem::paintCell(QPainter *painter, const QColorGroup &cg,
                                  int column, int width, int align)
{
	painter->save();
	painter->setFont(m_font);
	KListViewItem::paintCell(painter, cg, column, width, align);
	painter->restore();
}

int FancyListViewItem::width(const QFontMetrics&, const QListView *lv, int column) const
{
	int width;
	QFontMetrics fm2(m_font);
	width = QListViewItem::width(fm2, lv, column);
	return width;
}
