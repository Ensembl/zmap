/*  File: ZWTrackCollection.cpp
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

#include "ZWTrackCollection.h"
#include "ZWTrackSeparator.h"
#include "ZWTrackSimple.h"
#include "ZFeature.h"
#include "Utilities.h"
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

#include <QtWidgets>
#include <algorithm>


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZWTrackCollection>::m_name ("ZWTrackCollection") ;
const QColor ZWTrackCollection::m_default_color(Qt::transparent) ;

ZWTrackCollection::ZWTrackCollection(ZInternalID id)
    : ZWTrack(id, m_default_color),
      Util::InstanceCounter<ZWTrackCollection>(),
      Util::ClassName<ZWTrackCollection>(),
      m_track_ordering(),
      m_track_map(),
      m_children()
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZWTrackCollection::ZWTrackCollection() ; can not create without instance of QApplication existing")) ;
    if (!m_id)
        throw std::runtime_error(std::string("ZWTrackCollection::ZWTrackCollection() ; id may not be set to 0 ")) ;
    setPos(QPointF(0.0,0.0)) ;
}

ZWTrackCollection::~ZWTrackCollection()
{
}

QRectF ZWTrackCollection::boundingRect() const
{
    return childrenBoundingRect();
}

bool ZWTrackCollection::addChildItem(QGraphicsItem *item)
{
    bool result = false ;
    ZWTrack *track = dynamic_cast<ZWTrack*>(item) ;
    if (track && !hasChildItem(track->getID()))
    {
        track->setParentItem(this) ;
        connect(track, SIGNAL(internalReorderChange()), SLOT(internalReorder())) ;
        if (std::find(m_track_ordering.begin(), m_track_ordering.end(), track->getID()) == m_track_ordering.end())
        {
            m_track_ordering.push_back(track->getID()) ;
        }
        result = true ;
    }
    return result ;
}

bool ZWTrackCollection::hasChildItem(ZInternalID id ) const
{
    return m_children.find(id) != m_children.end() ;
}


void ZWTrackCollection::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    internalReorder() ;
    QGraphicsItem::mouseDoubleClickEvent(event) ;
}

// this puts in ordering in whatever the child tracks appear in the
// container returned by childItems()
void ZWTrackCollection::setDefaultTrackOrdering()
{
    m_track_ordering.clear() ;

    QList<QGraphicsItem*>  list = childItems() ;
    for (auto it = list.begin() ; it != list.end() ; ++it )
    {
        ZWTrack* track = dynamic_cast<ZWTrack*>(*it) ;
        if (track)
        {
            m_track_ordering.push_back(track->getID()) ;
        }
    }
    internalReorder() ;
}

// check that every track in the container has an entry in the
// argument data vector
bool ZWTrackCollection::setTrackOrdering(const std::vector<ZInternalID> &data)
{
    bool result = true ;
    QList<QGraphicsItem*> list = childItems() ;
    for (auto it = list.begin() ; it != list.end() ; ++it )
    {
        ZWTrack* track = dynamic_cast<ZWTrack*>(*it) ;
        if (track)
        {
            if (std::find(data.begin(), data.end(), track->getID()) == data.end())
            {
                result = false ;
                break ;
            }
        }
    }
    if (result)
    {
        m_track_ordering = data ;
        internalReorder() ;
    }
    return result ;
}

void ZWTrackCollection::setTrackMap(const std::multimap<ZInternalID, ZWTrack*> & data)
{
    m_track_map = data ;
}

ZWTrackSeparator* ZWTrackCollection::findStrandSeparator() const
{
    ZWTrackSeparator * separator = Q_NULLPTR ;
    QList<QGraphicsItem*> list = childItems() ;
    for (auto it = list.begin() ; it != list.end() ; ++it)
    {
        if (*it && (separator = dynamic_cast<ZWTrackSeparator*>(*it)))
            break ;
    }
    return separator ;
}

std::set<ZWTrack*> ZWTrackCollection::findTrack(ZInternalID id) const
{
    std::set<ZWTrack*> data ;
    auto pit = m_track_map.equal_range(id) ;
    for (auto it = pit.first ; it != pit.second ; ++it)
        data.insert(it->second) ;
    return data ;
}

// OK, so this is the actual reordering...
void ZWTrackCollection::internalReorder()
{
    // this version runs over all children including the
    // strand separator
    bool action = false ;
    QPointF pos ;
    ZWTrackSimple * track = Q_NULLPTR ;
    ZWTrackSimple::Strand strand = ZWTrackSimple::Strand::Invalid ;
    prepareGeometryChange() ;

    // we first find the strand separator and get its width; the offset of the
    // positive side starts at zero and the offset of the negative side starts
    // at the width of the separator
    ZWTrackSeparator* separator = findStrandSeparator() ;
    qreal offset_plus = 0.0,
            offset_minus = 0.0 ;
    if (separator)
    {
        offset_minus = separator->boundingRect().height() ;
    }

    // now we just do the same thing but in the order
    // in which the ids are present in the m_track_ordering container.
    offset_plus = 0.0 ;
    offset_minus = 0.0 ;
    if (separator)
    {
        offset_minus = separator->boundingRect().height() ;
    }
    for (auto it = m_track_ordering.begin() ; it != m_track_ordering.end() ; ++it, action=true)
    {
        ZInternalID current_id = *it ;
        auto pit = m_track_map.equal_range(current_id) ;
        for (auto ti = pit.first ; ti != pit.second ; ++ti )
        {
            if ((track = dynamic_cast<ZWTrackSimple*>(ti->second)))
            {
                pos = track->pos() ;
                strand = track->getStrand() ;
                if (strand == ZWTrackSimple::Strand::Plus || strand == ZWTrackSimple::Strand::Independent)
                {
                    offset_plus -= track->boundingRect().height() ;
                    pos.setY(offset_plus) ;
                }
                else if (strand == ZWTrackSimple::Strand::Minus)
                {
                    pos.setY(offset_minus) ;
                    offset_minus += track->boundingRect().height() ;
                }
                track->setPos(pos) ;
            }
        }
    }


    // send notification
    if (action)
    {
        emit internalReorderChange() ;
    }
}

ZWTrackCollection* ZWTrackCollection::newInstance(ZInternalID id )
{
    ZWTrackCollection* collection = Q_NULLPTR ;

    try
    {
        collection = new ZWTrackCollection(id) ;
    }
    catch (...)
    {
        collection = Q_NULLPTR ;
    }

    return collection ;
}


QVariant ZWTrackCollection::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemChildAddedChange)
    {
        QGraphicsItem *item = Q_NULLPTR ;
        if (value.canConvert<QGraphicsItem*>())
        {
            item = value.value<QGraphicsItem*>() ;
            ZWTrack* track = dynamic_cast<ZWTrack*>(item) ;
            if (track)
            {
                m_children.insert(track->getID()) ;
            }
        }
    }
    if (change == ItemChildRemovedChange)
    {
        QGraphicsItem *item = Q_NULLPTR ;
        if (value.canConvert<QGraphicsItem*>())
        {
            item = value.value<QGraphicsItem*>() ;
            ZWTrack *track = dynamic_cast<ZWTrack*>(item) ;
            if (track)
            {
                m_children.erase(track->getID()) ;
            }
        }
    }
    return QGraphicsItem::itemChange(change, value) ;
}

} // namespace GUI

} // namespace ZMap
