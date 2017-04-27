/*  File: ZFeatureGeneral.cpp
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

#include "ZFeatureGeneral.h"
#include "ZDebug.h"
#include <QtWidgets>
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZFeatureGeneral>::m_name ("ZFeatureGeneral") ;

ZFeatureGeneral::ZFeatureGeneral(ZInternalID id,
                                 ZInternalID featureset_id,
                                 const ZFeatureBounds &bounds,
                                 ZFeaturePainter *feature_painter )
    : ZFeature(id, featureset_id, bounds, feature_painter ),
      Util::InstanceCounter<ZFeatureGeneral>(),
      Util::ClassName<ZFeatureGeneral>()
{
}

ZFeatureGeneral::ZFeatureGeneral(ZInternalID id,
                                 ZInternalID featureset_id,
                                 const std::vector<ZFeatureBounds> & subparts,
                                 ZFeaturePainter *feature_painter )
    : ZFeature(id, featureset_id, 0.0, 0.0, feature_painter),
      Util::InstanceCounter<ZFeatureGeneral>(),
      Util::ClassName<ZFeatureGeneral>()
{
    if (subparts.size() == 0)
        throw std::runtime_error(std::string("ZFeatureGeneral::ZFeatureGeneral() ; must have 1 or more subparts ")) ;
    ZFeatureBounds bounds(subparts[0].start, subparts[subparts.size()-1].end ) ;
    if (!setInternalData(bounds))
        throw std::runtime_error(std::string("ZFeatureGeneral::ZFeatureGeneral() ; setInternalData() failed .")) ;
    if (subparts.size() > 1)
    {
        qreal start = subparts[0].start ;
        ZFeatureBounds subpart;
        for (unsigned int i=0 ; i<subparts.size() ; ++i)
        {
            subpart = subparts[i] ;
            subpart.start -= start ;
            subpart.end -= start ;
            ZFeature *subfeature = new ZFeatureGeneral(m_id, featureset_id, subpart, m_feature_painter) ;
            subfeature->setParentItem(this) ;
            subfeature->setZValue(static_cast<qreal>(i)) ;
        }
    }
}

ZFeatureGeneral::~ZFeatureGeneral()
{
}

void ZFeatureGeneral::paintDefault(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option) ;
    Q_UNUSED(widget) ;

    ZFeatureBounds subpart ;
    const QColor *color = isSelected() ? &ZFeature::ZWhite : &ZFeature::ZBlack ;
    unsigned int n_items = getNumSubparts() ;

    // draw linking bits
    if (n_items > 1)
    {
        QPen pen_old = painter->pen(),
                pen ;
        qreal w2 = m_width/2.0 ;
        pen.setWidth(0.0) ;
        pen.setColor(*color) ;
        painter->setPen(pen) ;
        QPointF p1(0.0, w2), p2(0.0, w2) ;
        ZFeatureBounds previous = getSubpart(0) ;
        for (unsigned int i=1; i<n_items; ++i)
        {
            subpart = getSubpart(i) ;
            p1.setX(previous.end) ;
            p2.setX(subpart.start) ;
            painter->drawLine(p1, p2) ;
            previous = subpart ;
        }
        painter->setPen(pen_old) ;
    }

    // draw boxes
    for (unsigned int i=0; i<n_items; ++i)
    {
        subpart = getSubpart(i) ;
        QRectF rect(subpart.start, 0.0, subpart.end-subpart.start, m_width) ;
        painter->fillRect(rect, *color) ;
    }
}

} // namespace GUI

} // namespace ZMap
