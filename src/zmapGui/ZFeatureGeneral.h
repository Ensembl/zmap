/*  File: ZFeatureGeneral.h
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

#ifndef ZFEATUREGENERAL_H
#define ZFEATUREGENERAL_H

#include "ZFeature.h"
#include "InstanceCounter.h"
#include "ClassName.h"

namespace ZMap
{

namespace GUI
{

class ZFeaturePainter ;

// a compound feature that has instances of itself as children
// default drawing is with straight line connectors
class ZFeatureGeneral: public ZFeature,
        public Util::InstanceCounter<ZFeatureGeneral>,
        public Util::ClassName<ZFeatureGeneral>
{
public:

    ZFeatureGeneral(ZInternalID id,
                    ZInternalID featureset_id,
                    const ZFeatureBounds &bounds,
                    ZFeaturePainter *feature_painter =  Q_NULLPTR) ;
    ZFeatureGeneral(ZInternalID id,
                    ZInternalID featureset_id,
                    const std::vector<ZFeatureBounds> & subparts,
                    ZFeaturePainter *feature_painter = Q_NULLPTR ) ;
    ~ZFeatureGeneral() ;

    size_t getNumSubparts() const
    {
        size_t i = static_cast<size_t>(childItems().size()) ;
        return i ? i : 1  ;
    }
    ZFeatureBounds getSubpart(unsigned int i) const
    {
        QList<QGraphicsItem*> children = childItems() ;
        ZFeature *feature = Q_NULLPTR ;
        if (children.size())
        {
            feature = dynamic_cast<ZFeature*>(children.at(i)) ;
        }
        if (feature)
            return ZFeatureBounds(feature->getStart(), feature->getEnd()) ;
        else
            return ZFeature::getSubpart(i) ;
    }

protected:

private:
    static const std::string m_name ;

    void paintDefault(QPainter *painter,
                      const QStyleOptionGraphicsItem *option,
                      QWidget *widget) override ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZFEATUREGENERAL_H
