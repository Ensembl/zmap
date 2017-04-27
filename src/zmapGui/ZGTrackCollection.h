/*  File: ZGTrackCollection.h
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

#ifndef ZGTRACKCOLLECTION_H
#define ZGTRACKCOLLECTION_H

#include "ZInternalID.h"
#include "ZGTrack.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <ostream>

namespace ZMap
{

namespace GUI
{

typedef std::map<ZInternalID,ZGTrack> ZGTrackCollectionContainerType ;
typedef std::pair<ZInternalID,ZGTrack> ZGTrackCollectionElementType ;

// containter for tracks

class ZGTrackCollection: public Util::InstanceCounter<ZGTrackCollection>,
        public Util::ClassName<ZGTrackCollection>
{
public:
    class iterator
    {
    public:
        iterator() ;
        iterator(ZGTrackCollection *container,
                 const ZGTrackCollectionContainerType::iterator & it) ;
        iterator(const iterator& it) ;
        ~iterator() ;
        iterator& operator=(const iterator& it) ;
        bool operator==(const iterator& rhs) ;
        bool operator!=(const iterator& rhs) ;
        iterator& operator++() ;
        iterator operator++(int) ;
        ZGTrack& operator*() const ;
        ZGTrack& featureset() const ;
        ZGTrack* operator->() const ;
    private:
        ZGTrackCollection *m_container ;
        ZGTrackCollectionContainerType::iterator m_iterator ;
    };
    class const_iterator
    {
    public:
        const_iterator() ;
        const_iterator(const ZGTrackCollection* container,
                       const ZGTrackCollectionContainerType::const_iterator & it ) ;
        const_iterator(const const_iterator& it) ;
        ~const_iterator() ;
        const_iterator& operator=(const const_iterator& it) ;
        bool operator==(const const_iterator& rhs) ;
        bool operator!=(const const_iterator& rhs) ;
        const_iterator& operator++() ;
        const_iterator operator++(int) ;
        const ZGTrack& operator*() const ;
        const ZGTrack& featureset() const ;
        const ZGTrack* operator->() const ;
    private:
        const ZGTrackCollection *m_container ;
        ZGTrackCollectionContainerType::const_iterator m_iterator ;
    };

    friend std::ostream& operator<<(std::ostream& str, const ZGTrackCollection& track_collection) ;

    ZGTrackCollection();
    ZGTrackCollection(const ZGTrackCollection& collection) ;
    ZGTrackCollection& operator=(const ZGTrackCollection & collection) ;
    ~ZGTrackCollection() ;

    bool contains(ZInternalID id ) const ;
    bool add(const ZGTrack& find) ;
    bool remove(ZInternalID id) ;
    size_t size() const ;
    std::set<ZInternalID> getIDs() const ;

    // reference access
    const ZGTrack& operator[](ZInternalID id) const ;
    ZGTrack& operator[](ZInternalID id) ;

    // iterator access to the container
    ZGTrackCollection::iterator begin() ;
    ZGTrackCollection::iterator end() ;
    ZGTrackCollection::iterator find(ZInternalID id) ;
    ZGTrackCollection::const_iterator begin() const ;
    ZGTrackCollection::const_iterator end() const ;
    ZGTrackCollection::const_iterator find(ZInternalID id) const ;

private:
    ZGTrackCollectionContainerType m_tracks ;
};

std::ostream& operator<<(std::ostream& str, const ZGTrackCollection& track_collection) ;

} // namespace GUI

} // namespace ZMap


#endif // ZGTRACKCOLLECTION_H
