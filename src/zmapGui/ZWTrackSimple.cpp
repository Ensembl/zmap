/*  File: ZWTrackSimple.cpp
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

#include "ZWTrackSimple.h"
#include "ZFeature.h"
#include "ZDebug.h"
#include "Utilities.h"
#include <QtWidgets>
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif
#include <algorithm>


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZWTrackSimple>::m_name("ZWTrackSimple") ;
const QString ZWTrackSimple::m_name_base("ZWTrackSimple_") ;
const QColor ZWTrackSimple::m_default_color(Qt::transparent) ;
const qreal ZWTrackSimple::m_default_length(10000.0),
    ZWTrackSimple::m_default_width(20.0) ;

QString ZWTrackSimple::getNameFromID(ZInternalID id)
{
    QString str ;
    return m_name_base + str.setNum(id) ;
}

ZWTrackSimple::ZWTrackSimple(ZInternalID id)
    : ZWTrack(id, m_default_color),
      Util::InstanceCounter<ZWTrackSimple>(),
      Util::ClassName<ZWTrackSimple>(),
      m_isBumped(false),
      m_strand(Strand::Plus),
      m_children()
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZWTrackSimple::ZWTrackSimple() ; can not create without instance of qApplication existing ")) ;
    if (!m_id)
        throw std::runtime_error(std::string("ZWTrackSimple::ZWTrackSimple() ; can not create instance with id as 0")) ;
    setPos(QPointF(0.0, 0.0)) ;
}

ZWTrackSimple::~ZWTrackSimple()
{
}

ZWTrackSimple* ZWTrackSimple::newInstance(ZInternalID id)
{
    ZWTrackSimple* track = Q_NULLPTR ;

    try
    {
        track = new ZWTrackSimple(id) ;
    }
    catch (...)
    {
        track = Q_NULLPTR ;
    }

    if (track)
    {
        track->setObjectName(getNameFromID(id)) ;
    }

    return track ;
}

bool ZWTrackSimple::setStrand(Strand strand)
{
    if (strand == Strand::Invalid)
        return false ;
    m_strand = strand ;
    return true ;
}

QRectF ZWTrackSimple::boundingRect() const
{
    QRectF rect = childrenBoundingRect() ;
    if (rect.width() > 0.0 && rect.height() > 0.0)
    {
        return rect ;
    }
    else
    {
        return QRectF(QPointF(0.0, 0.0), QSizeF(m_default_length, m_default_width)) ;
    }
}

bool ZWTrackSimple::addChildItem(QGraphicsItem* item)
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

bool ZWTrackSimple::hasChildItem(ZInternalID id) const
{
    return m_children.find(id) != m_children.end() ;
}


void ZWTrackSimple::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    internalReorder() ;
    QGraphicsItem::mouseDoubleClickEvent(event) ;
}


void ZWTrackSimple::internalReorder()
{
    prepareGeometryChange() ;
    if (!m_isBumped)
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
            item->setZValue(static_cast<qreal>(i)) ;
        }
        m_isBumped = true ;
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
        m_isBumped = false ;
    }

    emit internalReorderChange();
}


// called for evey item change, we are only interested in when children are added
// or removed here... features are identified by id (er, that's what an id is innit)
//
// according to the docs, for ItemChildAddedChange and ItemChildRemovedChange the
// value should contain the appropriate QGraphicsItem*, but I test for this anyway;
// not clear if this is really guaranteed or not... in addition, because the value
// argument is const, you can't call value.convert(id), which is recommended in
// the QVariant documentation
//
QVariant ZWTrackSimple::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemChildAddedChange)
    {
        QGraphicsItem *item = Q_NULLPTR ;
        if (value.canConvert<QGraphicsItem*>())
        {
            item = value.value<QGraphicsItem*>() ;
            ZFeature* feature = dynamic_cast<ZFeature*>(item) ;
            if (feature)
            {
                m_children.insert(feature->getID()) ;
            }
        }
    }
    if (change == ItemChildRemovedChange)
    {
        QGraphicsItem *item = Q_NULLPTR ;
        if (value.canConvert<QGraphicsItem*>())
        {
            item = value.value<QGraphicsItem*>() ;
            ZFeature *feature = dynamic_cast<ZFeature*>(item) ;
            if (feature)
            {
                m_children.erase(feature->getID()) ;
            }
        }
    }
    return QGraphicsItem::itemChange(change, value) ;
}

} // namespace GUI

} // namespace ZMap
