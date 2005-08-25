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

#ifndef FANCYLISTVIEWITEM_H_
#define FANCYLISTVIEWITEM_H_

#include <klistview.h>

class FancyListViewItem : public KListViewItem
    {
    public:
        FancyListViewItem(QListView *parent, const QString &label1, const QString &label2, const QString &label3,
                                             const QString &label4, const QString &label5, const QString &label6)
            : KListViewItem(parent, label1, label2, label3, label4, label5, label6 )
        {}
    
        void paintCell(QPainter *painter, const QColorGroup &cg,
                       int column, int width, int align);
        int width(const QFontMetrics &fm, const QListView *lv, int column) const;
    
        QFont getFont() const { return m_font; }
        void setFont(const QFont &font) { m_font = font; }
    
    private:
        QFont m_font;
        
    };

#endif /*FANCYLISTVIEWITEM_H_*/
