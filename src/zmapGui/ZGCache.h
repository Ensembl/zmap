/*  File: ZGCache.h
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

#ifndef ZGCACHE_H
#define ZGCACHE_H

#include "ZInternalID.h"
#include "ZGSequence.h"
#include "ZGTrack.h"
#include "ZGFeatureset.h"
#include "ZGFeature.h"
#include "InstanceCounter.h"
#include "ClassName.h"

#include <cstddef>
#include <map>
#include <set>
#include <string>

// This is the highest level cache object, containing all sequences.
// Each of the sequence cache objects contains its tracks, featuresets,
// and mapping between them.


// (0) this is supposed to be associated with the top level pac abstraction
//     class to cache all relevant data
// (a) instance of sequence stores its id, tracks that it contains,
//     featuresets it contains, and a mapping for the association between the tracks
//     and featuresets. this needs to work in both directions


namespace ZMap
{

namespace GUI
{

typedef std::map<ZInternalID,ZGSequence> ZGCacheContainerType ;
typedef std::pair<ZInternalID,ZGSequence> ZGCacheElementType ;

class ZGCache: public Util::InstanceCounter<ZGCache>,
        public Util::ClassName<ZGCache>
{

public:
    class iterator
    {
    public:
        iterator() ;
        iterator(ZGCache *container,
                 const ZGCacheContainerType::iterator & it) ;
        iterator(const iterator& it) ;
        ~iterator() ;
        iterator& operator=(const iterator& it) ;
        bool operator==(const iterator& rhs) ;
        bool operator!=(const iterator& rhs) ;
        iterator& operator++() ;
        iterator operator++(int) ;
        ZGSequence& operator*() const ;
        ZGSequence& sequence() const ;
        ZGSequence* operator->() const ;
    private:
        ZGCache *m_container ;
        ZGCacheContainerType::iterator m_iterator ;
    };
    class const_iterator
    {
    public:
        const_iterator() ;
        const_iterator(const ZGCache* container,
                       const ZGCacheContainerType::const_iterator & it ) ;
        const_iterator(const const_iterator& it) ;
        ~const_iterator() ;
        const_iterator& operator=(const const_iterator& it) ;
        bool operator==(const const_iterator& rhs) ;
        bool operator!=(const const_iterator& rhs) ;
        const_iterator& operator++() ;
        const_iterator operator++(int) ;
        const ZGSequence& operator*() const ;
        const ZGSequence& sequence() const ;
        const ZGSequence* operator->() const ;
    private:
        const ZGCache *m_container ;
        ZGCacheContainerType::const_iterator m_iterator ;
    };

    ZGCache();
    ZGCache(const ZGCache &cache) ;
    ZGCache& operator=(const ZGCache &cache) ;
    ~ZGCache() ;

    bool contains(ZInternalID id) const ;
    bool add(ZInternalID id) ;
    bool remove(ZInternalID id) ;

    std::set<ZInternalID> getSequenceIDs() const ;
    size_t size() const ;

    // reference access to the container
    const ZGSequence& operator[](ZInternalID id) const ;
    ZGSequence& operator[](ZInternalID id) ;

    // iterator access to the container
    ZGCache::iterator begin() ;
    ZGCache::iterator end() ;
    ZGCache::iterator find(ZInternalID id) ;
    ZGCache::const_iterator begin() const ;
    ZGCache::const_iterator end() const ;
    ZGCache::const_iterator find(ZInternalID id) const ;


private:
    ZGCacheContainerType m_sequences ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGCACHE_H
