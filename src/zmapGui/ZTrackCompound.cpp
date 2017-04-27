/*  File: ZTrackCompound.cpp
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

#include "ZTrackCompound.h"
#include "ZTrack.h"
#include "ZFeature.h"
#include <QtWidgets>

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZTrackCompound>::m_name("ZTrackCompound") ;

ZTrackCompound::ZTrackCompound(const QColor &color)
    : ZTrack(color),
      Util::InstanceCounter<ZTrackCompound>(),
      Util::ClassName<ZTrackCompound>()
{
    setPos(QPointF(0.0, 0.0)) ;
}

ZTrackCompound::~ZTrackCompound()
{
}


// add a track only
bool ZTrackCompound::addChildItem(QGraphicsItem *item)
{
    bool result = false ;
    ZTrack *track = dynamic_cast<ZTrack*>(item) ;
    if (track)
    {
        track->setParentItem(this) ;
        internalReorder() ;
        result = true ;
    }
    return result ;
}


bool ZTrackCompound::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    bool result = false ;
    if (event)
    {
        if ((event->type() == QEvent::GraphicsSceneMousePress)
                || (event->type() == QEvent::GraphicsSceneMouseMove)
                || (event->type() == QEvent::GraphicsSceneMouseRelease) )
        {
            prepareGeometryChange();
        }
        else if (event->type() == QEvent::GraphicsSceneMouseDoubleClick )
        {
            ZTrack * track = Q_NULLPTR ;
            if ((track = dynamic_cast<ZTrack*>(watched)))
            {
                track->internalReorder() ;
                internalReorder() ;
                prepareGeometryChange();
                result = true ;
            }
        }
    }
    return result ;
}


void ZTrackCompound::internalReorder()
{
    QList<QGraphicsItem*> qlist = childItems() ;
    QGraphicsItem *item = Q_NULLPTR ;
    QPointF oPos(0.0, 0.0) ;
    qreal shift = 0.0 ;
    for (int i=0; i<qlist.length() ; ++i)
    {
        item = qlist.at(i) ;
        oPos = item->pos() ;
        oPos.setY(shift) ;
        item->setPos(oPos) ;
        shift += item->boundingRect().height() ;
    }
}


} // namespace GUI

} // namespace ZMap


