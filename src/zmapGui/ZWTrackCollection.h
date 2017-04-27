/*  File: ZWTrackCollection.h
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

#ifndef ZWTRACKCOMPOUND_H
#define ZWTRACKCOMPOUND_H

#include "ZWTrack.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>
#include <vector>
#include <map>
#include <set>

#include <QApplication>
#include <QColor>


namespace ZMap
{

namespace GUI
{

class ZGraphicsItem ;
class ZWTrackSeparator ;
typedef std::multimap<ZInternalID, ZWTrack*> ZTrackMap ;

class ZWTrackCollection : public ZWTrack,
        public Util::InstanceCounter<ZWTrackCollection>,
        public Util::ClassName<ZWTrackCollection>
{
    Q_OBJECT

public:
    static ZWTrackCollection* newInstance(ZInternalID id) ;

    ~ZWTrackCollection() ;

    bool addChildItem(QGraphicsItem*) override ;
    bool hasChildItem(ZInternalID id) const override ;
    QRectF boundingRect() const Q_DECL_OVERRIDE ;

    void setDefaultTrackOrdering() ;
    bool setTrackOrdering(const std::vector<ZInternalID> & data) ;
    std::vector<ZInternalID> getTrackOrdering() const {return m_track_ordering; }

    ZWTrackSeparator * findStrandSeparator() const ;
    std::set<ZWTrack*> findTrack(ZInternalID id) const ;

    void setTrackMap(const std::multimap<ZInternalID, ZWTrack*> & data) ;
    std::multimap<ZInternalID, ZWTrack*> getTrackMap() const {return m_track_map ; }

public slots:
    void internalReorder() override ;

signals:
    void internalReorderChange() ;

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*event) Q_DECL_OVERRIDE ;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) Q_DECL_OVERRIDE ;

private:
    static const QColor m_default_color ;

    ZWTrackCollection(ZInternalID id);
    ZWTrackCollection(const ZWTrackCollection&) = delete ;
    ZWTrackCollection& operator=(const ZWTrackCollection&) = delete ;

    std::vector<ZInternalID> m_track_ordering ;
    ZTrackMap m_track_map ;
    std::set<ZInternalID> m_children ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZWTRACKCOMPOUND_H
