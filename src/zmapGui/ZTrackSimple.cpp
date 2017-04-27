/*  File: ZTrackSimple.cpp
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

#include "ZTrackSimple.h"
#include "ZFeature.h"
#include "ZDebug.h"
#include <QtWidgets>
#include <algorithm>

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZTrackSimple>::m_name("ZTrackSimple") ;

ZTrackSimple::ZTrackSimple(const QColor &color)
    : ZTrack(color),
      Util::InstanceCounter<ZTrackSimple>(),
      Util::ClassName<ZTrackSimple>(),
      bBumped(false)
{
    setPos(QPointF(0.0, 0.0)) ;
}

ZTrackSimple::~ZTrackSimple()
{
}

bool ZTrackSimple::addChildItem(QGraphicsItem* item)
{
    bool result = false ;
    ZFeature *feature = dynamic_cast<ZFeature*>(item) ;
    if (feature)
    {
        feature->setParentItem(this) ;
        result = true ;
    }
    return result ;
}

void ZTrackSimple::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    prepareGeometryChange() ;
    internalReorder() ;
    QGraphicsItem::mouseDoubleClickEvent(event) ;
}


void ZTrackSimple::internalReorder()
{
    if (!bBumped)
    {
        QList<QGraphicsItem*> qlist = childItems() ;
        QGraphicsItem *item = Q_NULLPTR ;
        qreal shift = 0.0 ;
        QPointF oPos;
        std::sort(qlist.begin(), qlist.end(), ZFeatureAsGILessThan() ) ;
        for (int i=0; i<qlist.size(); ++i)
        {
            item = qlist.at(i) ;
            oPos = item->pos() ;
            oPos.setY(shift) ;
            item->setPos(oPos) ;
            shift += item->boundingRect().height() ; // that is, feature->getWidth()
        }
        bBumped = true ;
    }
    else
    {
        QList<QGraphicsItem*> qlist = childItems() ;
        QPointF oPos ;
        for (int i=0; i<qlist.size(); ++i)
        {
            oPos = qlist.at(i)->pos() ;
            oPos.setY(0.0) ;
            qlist.at(i)->setPos(oPos) ;
        }
        bBumped = false ;
    }
}

} // namespace GUI

} // namespace ZMap

