/*  File: ZFeature.h
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

#ifndef ZFEATURE_H
#define ZFEATURE_H

#include <QGraphicsItem>
#include <string>
#include <QColor>
#include "ZInternalID.h"
#include "ZFeaturePainter.h"
#include "ZFeatureBounds.h"

namespace ZMap
{

namespace GUI
{

//
// Sequence feature base class.
// Coordinates for construction are in world sequence coordinates. That is, the
// feature's sequence range are given by getStartS() and getEndS(), and the
// range of world coordinate values that this spans is given by getStart() and
// getEnd().
//
//
class ZFeature : public QGraphicsItem
{
public:
    static const QColor ZBlack, ZWhite ;
    static const qreal ZDefaultFeatureWidth ;

    ZFeature(ZInternalID id,
             ZInternalID featureset_id,
             const qreal &start,
             const qreal &end,
             ZFeaturePainter *feature_painter = nullptr) ;
    ZFeature(ZInternalID id,
             ZInternalID featureset_id,
             const ZFeatureBounds &bounds,
             ZFeaturePainter *feature_painter = nullptr) ;
    virtual ~ZFeature() {}

    QRectF boundingRect() const Q_DECL_OVERRIDE ;
    QPainterPath shape() const Q_DECL_OVERRIDE ;
    bool setBounds(const ZFeatureBounds &bounds) ;
    virtual size_t getNumSubparts() const {return static_cast<size_t>(1) ; }
    virtual ZFeatureBounds getSubpart(unsigned int) const
    {
        return ZFeatureBounds(0.0, m_length) ;
    }

    ZInternalID getID() const {return m_id ; }
    ZInternalID getFeaturesetID() const {return m_featureset_id ; }

    qreal getWidth() const
    {
        return m_width ;
    }

    qreal getStart() const
    {
        return pos().x() ;
    }

    qreal getEnd() const
    {
        return pos().x() + m_length ;
    }

    qreal getStartS() const
    {
        return pos().x() ;
    }

    qreal getEndS() const
    {
        return pos().x() + m_length - 1.0 ;
    }

    ZFeaturePainter *getFeaturePainter() const { return m_feature_painter; }
    void setFeaturePainter(ZFeaturePainter *feature_painter) ;

protected:

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) Q_DECL_OVERRIDE ;

    bool setInternalData(const ZFeatureBounds & bounds) ;
    void setWidthFromPainter() ;

    ZInternalID m_id,
        m_featureset_id ;
    qreal m_length ;
    qreal m_width ;
    ZFeaturePainter *m_feature_painter ;

private:

    virtual void paintDefault(QPainter *painter,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget) = 0 ;
};

class ZFeatureLessThan
{
public:
    bool operator()(ZFeature *f1, ZFeature *f2) const ;
} ;

class ZFeatureAsGILessThan
{
public:
    bool operator()(QGraphicsItem*i1, QGraphicsItem*i2) const ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZFEATURE_H
