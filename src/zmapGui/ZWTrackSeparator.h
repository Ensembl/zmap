/*  File: ZWTrackSeparator.h
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

#ifndef ZWTRACKSEPARATOR_H
#define ZWTRACKSEPARATOR_H

#include <cstddef>
#include <string>
#include <set>
#include <QApplication>
#include <QColor>
#include "ZWTrack.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZFeaturePainter.h"


namespace ZMap
{

namespace GUI
{

class ZGraphicsItem ;

class ZWTrackSeparator : public ZWTrack,
        public Util::InstanceCounter<ZWTrackSeparator>,
        public Util::ClassName<ZWTrackSeparator>
{

    Q_OBJECT

public:
    static ZWTrackSeparator* newInstance() ;
    static QString getNameFromID(ZInternalID id) ;

    ~ZWTrackSeparator() ;

    bool addChildItem(QGraphicsItem*) override ;
    bool hasChildItem(ZInternalID id ) const override ;

    QRectF boundingRect() const Q_DECL_OVERRIDE ;

    static ZInternalID getDefaultID() {return m_default_id ; }
    bool setLength(const qreal& value) ;
    qreal getLength() const {return m_length ; }
    bool setWidth(const qreal & value) ;
    qreal getWidth() const {return m_width ; }

public slots:
    void internalReorder() override ;

signals:
    void internalReorderChange() ;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) Q_DECL_OVERRIDE ;

private:
    static const QColor m_default_color ;
    static const qreal m_default_length,
        m_default_width ;
    static const QString m_name_base ;
    static const ZInternalID m_default_id ;

    ZWTrackSeparator() ;
    ZWTrackSeparator(const ZWTrackSeparator&) = delete ;
    ZWTrackSeparator& operator=(const ZWTrackSeparator&) = delete ;

    qreal m_length, m_width ;
    std::set<ZInternalID> m_children ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZWTRACKSEPARATOR_H
