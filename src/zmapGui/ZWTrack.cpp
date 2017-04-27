/*  File: ZWTrack.cpp
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

#include "ZWTrack.h"
#include <QtWidgets>
#include <QStyleOptionGraphicsItem>
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

const QColor ZWTrack::m_default_highlight_color(Qt::yellow) ;
const int ZWTrack::m_qgi_id(qMetaTypeId<QGraphicsItem*>()) ;

ZWTrack::ZWTrack(ZInternalID id, const QColor &color)
    : QGraphicsObject(),
      m_id(id),
      m_color(color)
{
    setFlags( // ItemIsSelectable |
              ItemSendsGeometryChanges ) ;
    setFiltersChildEvents(true) ;
}


QPainterPath ZWTrack::shape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

void ZWTrack::setColor(const QColor &color)
{
    m_color = color ;
    update() ;
}

void ZWTrack::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    if (!painter || !option)
        return ;

    QColor fillColor = (option->state & QStyle::State_Selected) ? m_default_highlight_color : m_color ;
    painter->fillRect(boundingRect(), fillColor);
}

QVariant ZWTrack::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemChildAddedChange)
    {
        prepareGeometryChange() ;
    }
    else if (change == ItemChildRemovedChange)
    {
        prepareGeometryChange() ;
    }
    return QGraphicsItem::itemChange(change, value) ;
}

} // namespace GUI

} // namespace ZMap

