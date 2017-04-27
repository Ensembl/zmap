/*  File: ZFeatureBasic.cpp
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

#include "ZFeatureBasic.h"
#include "ZFeaturePainter.h"
#include <QtWidgets>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZFeatureBasic>::m_name ("ZFeatureBasic") ;

ZFeatureBasic::ZFeatureBasic(ZInternalID id,
                             ZInternalID featureset_id,
                             const qreal &start,
                             const qreal &end,
                             ZFeaturePainter *feature_painter)
    : ZFeature(id, featureset_id, start, end, feature_painter),
      Util::InstanceCounter<ZFeatureBasic>(),
      Util::ClassName<ZFeatureBasic>()
{

}

ZFeatureBasic::ZFeatureBasic(ZInternalID id,
                             ZInternalID featureset_id,
                             const ZFeatureBounds &bounds,
                             ZFeaturePainter *feature_painter)
    : ZFeature(id, featureset_id, bounds, feature_painter),
      Util::InstanceCounter<ZFeatureBasic>(),
      Util::ClassName<ZFeatureBasic>()
{

}

ZFeatureBasic::~ZFeatureBasic()
{

}

void ZFeatureBasic::paintDefault(QPainter *painter,
                                 const QStyleOptionGraphicsItem *option,
                                 QWidget *widget)
{
    Q_UNUSED(option) ;
    Q_UNUSED(widget) ;
    painter->fillRect(boundingRect(), isSelected() ? ZFeature::ZWhite : ZFeature::ZBlack );
}

} // namespace GUI

} // namespace ZMap
