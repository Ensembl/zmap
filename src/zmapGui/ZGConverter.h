/*  File: ZGConverter.h
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

#ifndef ZGCONVERTER_H
#define ZGCONVERTER_H

#include <QtGlobal>
#include "ZInternalID.h"
#include <cstddef>
#include <vector>

//
// This is used to convert from cache objects (the ZG* set of classes)
// into QT-based classes (ZFeature, ZTrack, etc).
//

QT_BEGIN_NAMESPACE
class QColor ;
QT_END_NAMESPACE

namespace ZMap
{

namespace GUI
{

// GUI/presenter classes
class ZFeature ;
class ZWTrack ;
class ZFeatureBounds ;
class ZFeaturePainter ;

// Cache/abstraction classes
class ZGFeature ;
class ZGTrack ;
class ZGColor ;
class ZGFeatureBounds ;
class ZGStyle ;

class ZGConverter
{
public:
    static ZFeature* convertFeature(ZInternalID featureset_id, const ZGFeature& feature) ;
    static ZWTrack* convertTrack(const ZGTrack& track) ;
    static QColor convertColor(const ZGColor& color) ;
    static ZFeatureBounds convertBounds(const ZGFeatureBounds& bounds) ;
    static std::vector<ZFeatureBounds> convertSubparts(const std::vector<ZGFeatureBounds>& bounds) ;
    static ZFeaturePainter* convertStyle(const ZGStyle &style) ;
private:
    ZGConverter() {}
    ZGConverter(const ZGConverter& ) = delete ;
    ZGConverter& operator=(const ZGConverter&) = delete ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGCONVERTER_H
