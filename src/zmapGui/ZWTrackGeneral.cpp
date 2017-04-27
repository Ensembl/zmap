/*  File: ZWTrackGeneral.cpp
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

#include "ZWTrackGeneral.h"
#include "ZFeature.h"
#include <QtWidgets>


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZWTrackGeneral>::m_name ("ZWTrackGeneral") ;
const QColor ZWTrackGeneral::m_default_color(Qt::transparent) ;

ZWTrackGeneral::ZWTrackGeneral(ZInternalID id)
    : ZWTrack(id, m_default_color),
      Util::ClassName<ZWTrackGeneral>(),
      Util::InstanceCounter<ZWTrackGeneral>()
{
    if (!m_id)
        throw std::runtime_error(std::string("ZWTrackGeneral::ZWTrackGeneral() ; id may not be set to 0 ")) ;
    setPos(QPointF(0.0, 0.0)) ;
}

ZWTrackGeneral::~ZWTrackGeneral()
{
}

QRectF ZWTrackGeneral::boundingRect() const
{
    return childrenBoundingRect();
}

bool ZWTrackGeneral::addChildItem(QGraphicsItem*)
{
    bool result = false ;
//    if (m_isCompound)
//    {
//        ZWTrack *track = dynamic_cast<ZWTrack*>(item) ;
//        if (track)
//        {
//            track->setParentItem(this) ;
//            result = true ;
//        }
//    }
//    else
//    {
//        ZFeature *feature = dynamic_cast<ZFeature*>(item) ;
//        if (feature)
//        {
//            feature->setParentItem(this) ;
//            result = true ;
//        }
//    }
    return result ;
}

bool ZWTrackGeneral::hasChildItem(ZInternalID) const
{
    return false ;
}

void ZWTrackGeneral::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    internalReorder() ;
    QGraphicsItem::mouseDoubleClickEvent(event) ;
}

void ZWTrackGeneral::internalReorder()
{
    prepareGeometryChange() ;

    emit internalReorderChange() ;
}


QVariant ZWTrackGeneral::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGraphicsItem::itemChange(change, value) ;
}

} // namespace GUI

} // namespace ZMap

