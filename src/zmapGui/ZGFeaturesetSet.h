/*  File: ZGFeaturesetSet.h
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

#ifndef ZGFEATURESETSET_H
#define ZGFEATURESETSET_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGFeature.h"
#include "ZGStyle.h"
#include <cstddef>
#include <set>
#include <string>

namespace ZMap
{

namespace GUI
{

// this is a version of the above using stl set container
class ZGFeaturesetSetReader ;

typedef std::set<ZGFeature,ZGFeatureCompID> ZGFeaturesetSetContainerType ;

class ZGFeaturesetSet: public Util::InstanceCounter<ZGFeaturesetSet>,
        public Util::ClassName<ZGFeaturesetSet>
{
    friend class ZGFeaturesetSetReader ;
public:
    ZGFeaturesetSet();
    ZGFeaturesetSet(ZInternalID id) ;
    ZGFeaturesetSet(const ZGFeaturesetSet& featureset) ;
    ZGFeaturesetSet& operator=(const ZGFeaturesetSet& featureset) ;
    ~ZGFeaturesetSet() ;

    const ZGFeature& operator[](ZInternalID id) const ;

    ZInternalID getID() const {return m_id ; }
    std::set<ZInternalID> getFeatureIDs() const ;

    ZGFeaturesetSetReader begin() ;
    ZGFeaturesetSetReader end() ;
    unsigned int nFeatures() const {return m_features.size() ; }

    void setStyle(const ZGStyle &style){m_style = style ; }
    ZGStyle getStyle() const {return m_style ; }

    bool containsFeature(ZInternalID id ) const ;
    bool containsFeature(const ZGFeature& feature) const ;
    bool addFeature(const ZGFeature& feature) ;
    bool removeFeature(ZInternalID id) ;

private:
    ZInternalID m_id ;
    ZGFeaturesetSetContainerType m_features ;
    ZGStyle m_style ;
};

class ZGFeaturesetSetReader: public Util::InstanceCounter<ZGFeaturesetSetReader>,
        public Util::ClassName<ZGFeaturesetSetReader>
{
public:
    ZGFeaturesetSetReader(ZGFeaturesetSet &featureset,
                       const ZGFeaturesetSetContainerType::iterator & iterator) ;
    ZGFeaturesetSetReader(const ZGFeaturesetSetReader &iterator) ;
    ZGFeaturesetSetReader& operator=(const ZGFeaturesetSetReader &iterator) ;

    bool operator==(const ZGFeaturesetSetReader& iterator) ;
    bool operator!=(const ZGFeaturesetSetReader& iterator) ;
    ZGFeaturesetSetReader& operator++() ;
    ZGFeaturesetSetReader operator++(int) ;
    const ZGFeature& operator*() const ;
    const ZGFeature& feature() const ;

private:
    ZGFeaturesetSet &m_featureset ;
    ZGFeaturesetSetContainerType::iterator m_iterator ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGFEATURESETSET_H
