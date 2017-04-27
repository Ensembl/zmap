/*  File: ZGTrackCollection.cpp
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

#include "ZGTrackCollection.h"
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGTrackCollection>::m_name("ZGTrackCollection") ;

ZGTrackCollection::ZGTrackCollection()
    : Util::InstanceCounter<ZGTrackCollection>(),
      Util::ClassName<ZGTrackCollection>(),
      m_tracks()
{
}

ZGTrackCollection::ZGTrackCollection(const ZGTrackCollection &collection)
    : Util::InstanceCounter<ZGTrackCollection>(),
      Util::ClassName<ZGTrackCollection>(),
      m_tracks(collection.m_tracks)
{
}

ZGTrackCollection& ZGTrackCollection::operator=(const ZGTrackCollection& collection)
{
    if (this != &collection)
    {
        m_tracks = collection.m_tracks ;
    }
    return *this ;
}

ZGTrackCollection::~ZGTrackCollection()
{
}

bool ZGTrackCollection::contains(ZInternalID id) const
{
    ZGTrackCollectionContainerType::const_iterator
            it =  m_tracks.find(id) ;
    if (it != m_tracks.end())
        return true ;
    else
        return false ;
}

bool ZGTrackCollection::remove(ZInternalID id)
{
    ZGTrackCollectionContainerType::iterator
            it = m_tracks.find(id) ;
    if (it != m_tracks.end())
    {
        m_tracks.erase(it) ;
        return true ;
    }
    return false ;
}

bool ZGTrackCollection::add(const ZGTrack& track)
{
    ZGTrackCollectionContainerType::iterator
            it = m_tracks.find(track.getID()) ;
    if (it == m_tracks.end())
    {
        m_tracks.insert(ZGTrackCollectionElementType(track.getID(),track)) ;
        return true ;
    }
    return false ;
}

size_t ZGTrackCollection::size() const
{
    return m_tracks.size() ;
}

std::set<ZInternalID> ZGTrackCollection::getIDs() const
{
    std::set<ZInternalID> data ;
    ZGTrackCollectionContainerType::const_iterator
            it = m_tracks.begin() ;
    for ( ; it!=m_tracks.end() ; ++it)
        data.insert(it->first) ;
    return data;
}

const ZGTrack& ZGTrackCollection::operator [](ZInternalID id ) const
{
    ZGTrackCollectionContainerType::const_iterator
            it = m_tracks.find(id) ;
    if (it == m_tracks.end())
        throw std::out_of_range(std::string("ZGTrackCollection::operator[]() const ; id not present in container ")) ;
    return it->second ;
}

ZGTrack& ZGTrackCollection::operator[](ZInternalID id)
{
    ZGTrackCollectionContainerType::iterator
            it = m_tracks.find(id) ;
    if (it == m_tracks.end())
        throw std::out_of_range(std::string("ZGTrackCollection::operator[]() ; id not present in container ")) ;
    return it->second ;
}

ZGTrackCollection::iterator ZGTrackCollection::find(ZInternalID id)
{
    return ZGTrackCollection::iterator(this, m_tracks.find(id)) ;
}

ZGTrackCollection::const_iterator ZGTrackCollection::find(ZInternalID id) const
{
    return ZGTrackCollection::const_iterator(this, m_tracks.find(id)) ;
}

ZGTrackCollection::iterator ZGTrackCollection::begin()
{
    return ZGTrackCollection::iterator(this, m_tracks.begin() ) ;
}

ZGTrackCollection::iterator ZGTrackCollection::end()
{
    return ZGTrackCollection::iterator(this, m_tracks.end() ) ;
}

ZGTrackCollection::const_iterator ZGTrackCollection::begin() const
{
    return ZGTrackCollection::const_iterator(this, m_tracks.begin() ) ;
}

ZGTrackCollection::const_iterator ZGTrackCollection::end() const
{
    return ZGTrackCollection::const_iterator(this, m_tracks.end() ) ;
}

std::ostream& operator<<(std::ostream& str, const ZGTrackCollection& track_collection)
{
    for (auto it=track_collection.begin() ; it!=track_collection.end() ; ++it)
    {
        str << it->getID() << " " ;
    }
    return str ;
}



////////////////////////////////////////////////////////////////////////////////
/// iterator access
////////////////////////////////////////////////////////////////////////////////
ZGTrackCollection::iterator::iterator()
    : m_container(nullptr),
      m_iterator()
{
}
ZGTrackCollection::iterator::iterator(ZGTrackCollection *container, const ZGTrackCollectionContainerType::iterator &it)
    : m_container(container),
      m_iterator(it)
{
    if (!m_container)
        throw std::runtime_error(std::string("ZGTrackCollection::iterator::iterator() ; container pointer may not be NULL ")) ;
}
ZGTrackCollection::iterator::iterator(const ZGTrackCollection::iterator& it)
    : m_container(it.m_container),
      m_iterator(it.m_iterator)
{
}
ZGTrackCollection::iterator& ZGTrackCollection::iterator::operator =(const ZGTrackCollection::iterator& it)
{
    if (this != &it)
    {
        m_container = it.m_container ;
        m_iterator = it.m_iterator ;
    }
    return *this ;
}
ZGTrackCollection::iterator::~iterator()
{
}

bool ZGTrackCollection::iterator::operator==(const iterator& it)
{
    if ((m_container == it.m_container) && (m_iterator==it.m_iterator))
        return true ;
    else
        return false ;
}
bool ZGTrackCollection::iterator::operator!=(const iterator& it)
{
    return !operator==(it) ;
}
ZGTrackCollection::iterator& ZGTrackCollection::iterator::operator ++()
{
    ++m_iterator ;
    return *this ;
}
ZGTrackCollection::iterator ZGTrackCollection::iterator::operator++(int)
{
    ZGTrackCollection::iterator temp(*this) ;
    ++m_iterator ;
    return temp ;
}
ZGTrack& ZGTrackCollection::iterator::operator*() const
{
    if (std::distance(m_iterator, m_container->m_tracks.end() ) <= 0)
        throw std::out_of_range(std::string("ZGTrackCollection::iterator::operator() ; out of range access")) ;
    return m_iterator->second ;
}
ZGTrack& ZGTrackCollection::iterator::featureset() const
{
    if (std::distance(m_iterator, m_container->m_tracks.end() ) <= 0)
        throw std::out_of_range(std::string("ZGTrackCollection::iterator::sequence() ; out of range access")) ;
    return m_iterator->second ;
}
ZGTrack* ZGTrackCollection::iterator::operator ->() const
{
    if (std::distance(m_iterator, m_container->m_tracks.end() ) <= 0)
        throw std::out_of_range(std::string("ZGTrackCollection::iterator::operator->() ; out of range access")) ;
    return &m_iterator->second ;
}




////////////////////////////////////////////////////////////////////////////////
/// const_iterator access
////////////////////////////////////////////////////////////////////////////////


ZGTrackCollection::const_iterator::const_iterator()
    : m_container(nullptr),
      m_iterator()
{
}
ZGTrackCollection::const_iterator::const_iterator(const ZGTrackCollection *container, const ZGTrackCollectionContainerType::const_iterator &it)
    : m_container(container),
      m_iterator(it)
{
    if (!m_container)
        throw std::runtime_error(std::string("ZGTrackCollection::const_iterator::const_iterator() ; container pointer may not be NULL")) ;
}
ZGTrackCollection::const_iterator::const_iterator(const ZGTrackCollection::const_iterator& it)
    : m_container(it.m_container),
      m_iterator(it.m_iterator)
{
}
ZGTrackCollection::const_iterator& ZGTrackCollection::const_iterator::operator =(const ZGTrackCollection::const_iterator & it)
{
    if (this != &it)
    {
        m_container = it.m_container ;
        m_iterator = it.m_iterator ;
    }
    return *this ;
}
ZGTrackCollection::const_iterator::~const_iterator()
{
}

bool ZGTrackCollection::const_iterator::operator==(const ZGTrackCollection::const_iterator& it)
{
    if ((m_container==it.m_container) && (m_iterator==it.m_iterator))
        return true ;
    else
        return false ;
}
bool ZGTrackCollection::const_iterator::operator!=(const ZGTrackCollection::const_iterator& it)
{
    return !operator==(it) ;
}
ZGTrackCollection::const_iterator& ZGTrackCollection::const_iterator::operator++()
{
    ++m_iterator ;
    return *this ;
}
ZGTrackCollection::const_iterator ZGTrackCollection::const_iterator::operator++(int)
{
    ZGTrackCollection::const_iterator temp(*this) ;
    ++m_iterator ;
    return temp ;
}
const ZGTrack& ZGTrackCollection::const_iterator::operator*() const
{
    if (std::distance(m_iterator, m_container->m_tracks.end() ) <= 0)
        throw std::out_of_range(std::string("ZGTrackCollection::const_iterator::operator() ; out of range access")) ;
    return m_iterator->second ;
}
const ZGTrack& ZGTrackCollection::const_iterator::featureset() const
{
    if (std::distance(m_iterator, m_container->m_tracks.end() ) <= 0)
        throw std::out_of_range(std::string("ZGTrackCollection::const_iterator::sequence() ; out of range access")) ;
    return m_iterator->second ;
}
const ZGTrack* ZGTrackCollection::const_iterator::operator ->() const
{
    if (std::distance(m_iterator, m_container->m_tracks.end() ) <= 0)
        throw std::out_of_range(std::string("ZGTrackCollection::const_iterator::operator->() ; out of range access")) ;
    return &m_iterator->second ;
}


} // namespace GUI

} // namespace ZMap


