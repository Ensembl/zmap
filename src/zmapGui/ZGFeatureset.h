/*  File: ZGFeatureset.h
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

#ifndef ZGFEATURESET_H
#define ZGFEATURESET_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGFeature.h"
#include "ZGStyle.h"
#include <map>
#include <set>
#include <string>
#include <cstddef>
// Caching classes for a featureset.

// Note that with these definitions, we do not need a comparator
// class for this map, since less<ZInternalID> (i.e. unsigned int)
// will be used by default. The class ZGFeatureCompID does this
// job nonetheless (see ZGFeature.h), and is there in case needed
// with some other container.

namespace ZMap
{

namespace GUI
{

typedef std::map<ZInternalID,ZGFeature> ZGFeaturesetContainerType ;
typedef std::pair<ZInternalID,ZGFeature> ZGFeaturesetElementType ;

class ZGFeatureset: public Util::InstanceCounter<ZGFeatureset>,
        public Util::ClassName<ZGFeatureset>
{
public:
    class iterator
    {
    public:
        iterator() ;
        iterator(ZGFeatureset *container,
                 const ZGFeaturesetContainerType::iterator &it) ;
        iterator(const iterator& it) ;
        iterator& operator=(const iterator& it) ;
        bool operator==(const iterator& rhs) const ;
        bool operator!=(const iterator& rhs) const ;
        iterator& operator++() ;
        iterator operator++(int) ;
        ZGFeature& operator*() const ;
        ZGFeature& feature() const ;
        ZGFeature* operator->() const ;
    private:
        ZGFeatureset *m_container ;
        ZGFeaturesetContainerType::iterator m_iterator ;
    };
    class const_iterator
    {
    public:
        const_iterator() ;
        const_iterator(const ZGFeatureset *container,
                       const ZGFeaturesetContainerType::const_iterator & it) ;
        const_iterator(const const_iterator & it) ;
        const_iterator& operator=(const const_iterator& it) ;
        bool operator==(const const_iterator& rhs) const ;
        bool operator!=(const const_iterator& rhs) const ;
        const_iterator& operator++() ;
        const_iterator operator++(int) ;
        const ZGFeature& operator*() const ;
        const ZGFeature& feature() const ;
        const ZGFeature* operator->() const ;
    private:
        const ZGFeatureset *m_container ;
        ZGFeaturesetContainerType::const_iterator m_iterator ;
    };

    ZGFeatureset();
    ZGFeatureset(ZInternalID id) ;
    ZGFeatureset(const ZGFeatureset& featureset) ;
    ZGFeatureset& operator=(const ZGFeatureset& featureset) ;
    ~ZGFeatureset() ;

    const ZGFeature& operator[](ZInternalID id) const ;
    ZGFeature& operator[](ZInternalID id) ;

    ZInternalID getID() const {return m_id ; }
    std::set<ZInternalID> getFeatureIDs() const ;

    iterator begin() ;
    iterator end() ;
    iterator find(ZInternalID id) ;
    const_iterator begin() const ;
    const_iterator end() const ;
    const_iterator find(ZInternalID id) const ;
    unsigned int size() const ;

    bool containsFeature(ZInternalID id ) const ;
    bool addFeature(const ZGFeature& feature) ;
    bool replaceFeature(const ZGFeature& feature) ;
    bool addFeatures(const ZGFeaturesetContainerType & features) ;
    bool removeFeature(ZInternalID id) ;
    bool removeFeatures(const std::set<ZInternalID> &dataset) ;
    void removeFeaturesAll() ;

    bool merge(const ZGFeatureset& featureset) ;

private:
    ZInternalID m_id ;
    ZGFeaturesetContainerType m_features ;
};

} // namespace GUI

} // namespace ZMap



#endif // ZGFEATURESET_H
