/*  File: ZGFeaturesetCollection.h
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

#ifndef ZGFEATURESETCOLLECTION_H
#define ZGFEATURESETCOLLECTION_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGFeatureset.h"

#include <map>
#include <set>
#include <ostream>
#include <cstddef>

namespace ZMap
{

namespace GUI
{

typedef std::map<ZInternalID,ZGFeatureset> ZGFeaturesetCollectionContainerType ;
typedef std::pair<ZInternalID,ZGFeatureset> ZGFeaturesetCollectionElementType ;

// containter for featuresets

class ZGFeaturesetCollection: public Util::InstanceCounter<ZGFeaturesetCollection>,
        public Util::ClassName<ZGFeaturesetCollection>
{
public:
    class iterator
    {
    public:
        iterator() ;
        iterator(ZGFeaturesetCollection *container,
                 const ZGFeaturesetCollectionContainerType::iterator & it) ;
        iterator(const iterator& it) ;
        ~iterator() ;
        iterator& operator=(const iterator& it) ;
        bool operator==(const iterator& rhs) ;
        bool operator!=(const iterator& rhs) ;
        iterator& operator++() ;
        iterator operator++(int) ;
        ZGFeatureset& operator*() const ;
        ZGFeatureset& featureset() const ;
        ZGFeatureset* operator->() const ;
    private:
        ZGFeaturesetCollection *m_container ;
        ZGFeaturesetCollectionContainerType::iterator m_iterator ;
    };
    class const_iterator
    {
    public:
        const_iterator() ;
        const_iterator(const ZGFeaturesetCollection* container,
                       const ZGFeaturesetCollectionContainerType::const_iterator & it ) ;
        const_iterator(const const_iterator& it) ;
        ~const_iterator() ;
        const_iterator& operator=(const const_iterator& it) ;
        bool operator==(const const_iterator& rhs) ;
        bool operator!=(const const_iterator& rhs) ;
        const_iterator& operator++() ;
        const_iterator operator++(int) ;
        const ZGFeatureset& operator*() const ;
        const ZGFeatureset& featureset() const ;
        const ZGFeatureset* operator->() const ;
    private:
        const ZGFeaturesetCollection *m_container ;
        ZGFeaturesetCollectionContainerType::const_iterator m_iterator ;
    };

    friend std::ostream& operator<<(std::ostream& str, const ZGFeaturesetCollection& featureset_collection) ;

    ZGFeaturesetCollection();
    ZGFeaturesetCollection(const ZGFeaturesetCollection& collection) ;
    ZGFeaturesetCollection& operator=(const ZGFeaturesetCollection & collection) ;
    ~ZGFeaturesetCollection() ;

    bool contains(ZInternalID id ) const ;
    bool add(const ZGFeatureset& find) ;
    bool remove(ZInternalID id) ;
    size_t size() const ;
    std::set<ZInternalID> getIDs() const ;

    // reference access
    const ZGFeatureset& operator[](ZInternalID id) const ;
    ZGFeatureset& operator[](ZInternalID id) ;

    // iterator access to the container
    ZGFeaturesetCollection::iterator begin() ;
    ZGFeaturesetCollection::iterator end() ;
    ZGFeaturesetCollection::iterator find(ZInternalID id) ;
    ZGFeaturesetCollection::const_iterator begin() const ;
    ZGFeaturesetCollection::const_iterator end() const ;
    ZGFeaturesetCollection::const_iterator find(ZInternalID id) const ;

private:
    ZGFeaturesetCollectionContainerType m_featuresets ;
};

std::ostream& operator<<(std::ostream& str, const ZGFeaturesetCollection& featureset_collection) ;

} // namespace GUI

} // namespace ZMap

#endif // ZGFEATURESETCOLLECTION_H
