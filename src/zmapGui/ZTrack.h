/*  File: ZTrack.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2016: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database user interface
 * and was written by
 *
 * Steve Miller (sm23@sanger.ac.uk)
 *
 * Description:
 *-------------------------------------------------------------------
 */

#ifndef ZTRACK_H
#define ZTRACK_H

#include <QGraphicsItem>
#include <QColor>
#include <string>


namespace ZMap
{

namespace GUI
{

class ZTrack : public QGraphicsItem
{
public:
    ZTrack(const QColor & color) ;
    virtual ~ZTrack() {}

    virtual std::string name() const = 0 ;
    virtual void internalReorder() = 0 ;
    virtual bool addChildItem(QGraphicsItem *) = 0 ;

    QRectF boundingRect() const Q_DECL_OVERRIDE ;
    QPainterPath shape() const Q_DECL_OVERRIDE ;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) Q_DECL_OVERRIDE ;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) Q_DECL_OVERRIDE ;

    QColor color ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZTRACK_H
