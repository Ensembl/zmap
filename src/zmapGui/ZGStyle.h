/*  File: ZGuiStyle.h
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

#ifndef ZGSTYLE_H
#define ZGSTYLE_H

#include "ZInternalID.h"
#include "ZGFeatureType.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGColor.h"
#include <cstddef>
#include <string>


namespace ZMap
{

namespace GUI
{

class ZGStyle: public Util::InstanceCounter<ZGStyle>,
        public Util::ClassName<ZGStyle>
{
public:

    ZGStyle() ;
    ZGStyle(ZInternalID id ) ;
    ZGStyle(ZInternalID id,
            const double &width,
            const ZGColor &c1,
            const ZGColor &c2,
            const ZGColor &c3,
            const ZGColor &c4,
            ZGFeatureType feature_type ) ;
    ZGStyle(const ZGStyle& style) ;
    ZGStyle& operator=(const ZGStyle &style) ;
    ~ZGStyle() ;

    ZInternalID getID() const {return m_id ; }
    double getWidth() const {return m_width ; }
    ZGColor getColorFill() const {return m_colorfill ; }
    ZGColor getColorOutline() const {return m_coloroutline ; }
    ZGColor getColorHighlight() const {return m_colorhighlight ; }
    ZGColor getColorCds() const {return m_colorcds ; }
    ZGFeatureType getFeatureType() const {return m_feature_type ; }

private:

    ZInternalID m_id ;
    double m_width ;
    ZGColor m_colorfill, m_coloroutline, m_colorhighlight, m_colorcds ;
    ZGFeatureType m_feature_type ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGSTYLE_H
