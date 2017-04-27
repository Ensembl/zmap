/*  File: ZGConverter.cpp
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

#include "ZGConverter.h"

// GUI/presenter classes
#include "ZFeature.h"
#include "ZWTrackSimple.h"
#include "ZFeatureBasic.h"
#include "ZFeatureComplex.h"
#include "ZFeatureBounds.h"
#include "ZFeaturePainter.h"
#include "ZFeaturePainterCompound.h"
#include "ZFeaturePainterBox01.h"
#include "ZFeaturePainterConn01.h"
#include "ZFeaturePainterConn02.h"
#include <QColor>

// Cache/abstraction classes
#include "ZGFeature.h"
#include "ZGTrack.h"
#include "ZGStyle.h"
#include "ZGColor.h"
#include "ZFeatureBounds.h"


namespace ZMap
{

namespace GUI
{

// create a feature from the
ZFeature* ZGConverter::convertFeature(ZInternalID featureset_id,
                                      const ZGFeature& feature)
{
    ZGFeatureType feature_type = feature.getType() ;
    ZInternalID feature_id = feature.getID() ;
    ZFeature *feature_item = Q_NULLPTR ;
    ZFeatureBounds bounds = ZGConverter::convertBounds(feature.getBounds()) ;
    if (feature_type == ZGFeatureType::Basic)
    {
        feature_item = new ZFeatureBasic(feature_id, featureset_id, bounds) ;
    }
    else if ((feature_type == ZGFeatureType::Transcript) || (feature_type == ZGFeatureType::Alignment))
    {
        feature_item = new ZFeatureComplex(feature_id, featureset_id, convertSubparts(feature.getSubparts())) ;
    }

    return feature_item ;
}

// simply create a white-colored track
ZWTrack* ZGConverter::convertTrack(const ZGTrack& track)
{
    return ZWTrackSimple::newInstance(track.getID()) ;
}


QColor ZGConverter::convertColor(const ZGColor &color)
{
    return QColor(color.getR(), color.getG(), color.getB(), color.getA()) ;
}

ZFeatureBounds ZGConverter::convertBounds(const ZGFeatureBounds &bounds)
{
    ZFeatureBounds b(bounds.start, bounds.end) ;
    return b ;
}

std::vector<ZFeatureBounds> ZGConverter::convertSubparts(const std::vector<ZGFeatureBounds>& bounds)
{
    std::vector<ZFeatureBounds> result(bounds.size(), ZFeatureBounds()) ;
    for (unsigned int i=0; i<bounds.size() ; ++i)
        result[i] = convertBounds(bounds[i]) ;
    return result ;
}


// convert a style into a feature painter
ZFeaturePainter* ZGConverter::convertStyle(const ZGStyle &style)
{
    ZFeaturePainter *feature_painter = nullptr ;
    ZGFeatureType feature_type = style.getFeatureType() ;
    QColor color_fill = convertColor(style.getColorFill()),
            color_highlight = convertColor(style.getColorHighlight()) ;
    double width = style.getWidth() ;

    if (feature_type == ZGFeatureType::Basic)
    {
        feature_painter = new ZFeaturePainterBox01(width, color_fill, color_highlight) ;
    }
    else if (feature_type == ZGFeatureType::Transcript)
    {
        feature_painter = new ZFeaturePainterCompound ;
        feature_painter->add(new ZFeaturePainterBox01(width, color_fill, color_highlight)) ;
        feature_painter->add(new ZFeaturePainterConn01(width, color_fill, color_highlight)) ;
    }
    else if (feature_type == ZGFeatureType::Alignment)
    {
        feature_painter = new ZFeaturePainterCompound ;
        feature_painter->add(new ZFeaturePainterBox01(width, color_fill, color_highlight)) ;
        feature_painter->add(new ZFeaturePainterConn02(width, color_fill, color_highlight)) ;
    }
    else
    {
        feature_painter = new ZFeaturePainterBox01(width, color_fill, color_highlight) ;
    }

    return feature_painter ;
}

} // namespace GUI

} // namespace ZMap
