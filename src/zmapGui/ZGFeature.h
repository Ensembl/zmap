/*  File: ZGFeature.h
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

#ifndef ZGFEATURE_H
#define ZGFEATURE_H

// Caching classes.

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGFeatureBounds.h"
#include "ZGFeatureType.h"
#include "ZGStrandType.h"

#include <vector>
#include <string>
#include <cstddef>

// provide subpart access through [] or iterator access; see
// featureset implementation

namespace ZMap
{

namespace GUI
{

class ZGFeature: public Util::InstanceCounter<ZGFeature>,
        public Util::ClassName<ZGFeature>
{
public:

    ZGFeature();
    ZGFeature(ZInternalID id) ;
    ZGFeature(ZInternalID id,
              ZGFeatureType type,
              ZGStrandType strand,
              const ZGFeatureBounds &bounds,
              const std::vector<ZGFeatureBounds> &subparts = std::vector<ZGFeatureBounds>()) ;
    ZGFeature(const ZGFeature& feature) ;
    ZGFeature& operator=(const ZGFeature &feature) ;
    ~ZGFeature() ;

    const ZGFeature* operator->() const {return this ; }

    ZInternalID getID() const {return m_id ; }
    const ZGFeatureBounds& getBounds() const {return m_bounds ;}
    ZGFeatureType getType() const {return m_type ; }
    ZGStrandType getStrand() const {return m_strand ; }
    const std::vector<ZGFeatureBounds>& getSubparts() const { return m_subparts ; }

private:
    ZInternalID m_id ;
    ZGFeatureBounds m_bounds ;
    ZGFeatureType m_type ;
    ZGStrandType m_strand ;
    std::vector<ZGFeatureBounds> m_subparts ;
};

class ZGFeatureCompID
{
public:
    bool operator()(const ZGFeature& f1, const ZGFeature &f2) const
    {
        if (f1.getID() < f2.getID())
            return true ;
        else
            return false ;
    }
};

} // namespace GUI

} // namespace ZMap


#endif // ZGFEATURE_H
