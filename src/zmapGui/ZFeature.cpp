/*  File: ZFeature.cpp
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

#include "ZFeature.h"
#include <QtWidgets>
#include <stdexcept>
#include <QString>


namespace ZMap
{

namespace GUI
{


// check of coordinate value
static bool checkValue(const double &value)
{
    static const double tolerance = 1.0e-12 ;
    double frac = fabs(value) - std::floor(fabs(value)) ;
    if (frac > 0.5)
    {
        if (1.0 - frac < tolerance)
            return true ;
    }
    else
    {
        if (frac < tolerance)
            return true ;
    }
    return false ;
}

const QColor ZFeature::ZBlack(Qt::black) ;
const QColor ZFeature::ZWhite(Qt::white) ;
const qreal ZFeature::ZDefaultFeatureWidth(10.0) ;

ZFeature::ZFeature(ZInternalID id,
                   ZInternalID featureset_id,
                   const ZFeatureBounds& bounds,
                   ZFeaturePainter *feature_painter )
    : QGraphicsItem(),
      m_id(id),
      m_featureset_id(featureset_id),
      m_length(0.0),
      m_width(ZDefaultFeatureWidth),
      m_feature_painter(feature_painter)
{
    if (!m_id)
        throw std::runtime_error(std::string("ZFeature::ZFeature() ; m_id may not be set to 0 ")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZFeature::ZFeature() ; m_featureset_id may not be set to be 0")) ;
    if (!setInternalData(bounds))
        throw std::runtime_error(std::string("ZFeature::ZFeature() ; setInternalData() failed. ") ) ;
    //setToolTip(QString().setNum(m_id)) ;
}

ZFeature::ZFeature(ZInternalID id,
                   ZInternalID featureset_id,
                   const qreal &start,
                   const qreal &end,
                   ZFeaturePainter *feature_painter)
    : QGraphicsItem(),
      m_id(id),
      m_featureset_id(featureset_id),
      m_length(0.0),
      m_width(ZDefaultFeatureWidth),
      m_feature_painter(feature_painter)
{
    if (!m_id)
        throw std::runtime_error(std::string("ZFeature::ZFeature() ; m_id may not be set to 0 ")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZFeature::ZFeature() ; m_featureset_id may not be set to 0")) ;
    if (!setInternalData(ZFeatureBounds(start, end)))
        throw std::runtime_error(std::string("ZFeature::ZFeature() ; setInternalData() failed. ") ) ;
    //setToolTip(QString().setNum(m_id)) ;
}


void ZFeature::paint(QPainter *painter,
           const QStyleOptionGraphicsItem *option,
           QWidget *widget)
{
    if (m_feature_painter)
    {
        m_feature_painter->paint(this, painter, option, widget) ;
    }
    else
    {
        paintDefault(painter, option, widget) ;
    }
}

bool ZFeature::setInternalData(const ZFeatureBounds& bounds)
{
    if (!setBounds(bounds))
        return false ;
    setWidthFromPainter();
    setFlags(ItemIsSelectable | ItemSendsGeometryChanges) ;
    setFlag(ItemIsMovable, false ) ;
    return true ;
}

void ZFeature::setFeaturePainter(ZFeaturePainter *feature_painter)
{
    prepareGeometryChange() ;
    m_feature_painter = feature_painter;
    setWidthFromPainter();
}

void ZFeature::setWidthFromPainter()
{
    if (!m_feature_painter)
    {
        m_width = ZDefaultFeatureWidth ;
    }
    else
    {
        m_width = m_feature_painter->getWidth() ;
    }
}

bool ZFeature::setBounds(const ZFeatureBounds &bounds)
{
    bool result = false ;
    if (   bounds.start<0.0
           || bounds.end<0.0
           || (bounds.start > bounds.end)
           || !checkValue(bounds.start)
           || !checkValue(bounds.end)
       )
    {
        return result ;
    }
    setPos(QPointF(bounds.start, 0.0) ) ;
    m_length = bounds.end - bounds.start + 1.0 ;
    result = true ;
    return result ;
}

QRectF ZFeature::boundingRect() const
{
    return QRectF(0.0, 0.0, m_length, m_width);
}

QPainterPath ZFeature::shape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

bool ZFeatureLessThan::operator()(ZFeature*f1, ZFeature*f2) const
{
    if (!f1 || !f2)
        return false ;
    if (f1->getStart() > f2->getStart())
        return false ;
    else if (f1->getStart() == f2->getStart())
    {
        if (f1->getEnd() > f2->getEnd() )
            return false ;
    }
    return true ;
}

bool ZFeatureAsGILessThan::operator()(QGraphicsItem*i1, QGraphicsItem*i2) const
{
    if (!i1 || !i2)
        return false ;
    qreal i1start = i1->pos().x(),
            i2start = i2->pos().x() ;
    if (i1start > i2start )
        return false ;
    else if (i1start == i2start )
    {
        if (i1start+i1->boundingRect().width() > i2start+i2->boundingRect().width() )
            return false ;
    }
    return true ;
}

} // namespace GUI

} // namespace ZMap
