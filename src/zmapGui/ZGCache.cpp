/*  File: ZGCache.cpp
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

#include "ZGCache.h"
#include <stdexcept>
#include <utility>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGCache>::m_name("ZGCache") ;

ZGCache::ZGCache()
    : Util::InstanceCounter<ZGCache>(),
      Util::ClassName<ZGCache>(),
      m_sequences()
{
}

ZGCache::ZGCache(const ZGCache &cache)
    : Util::InstanceCounter<ZGCache>(),
      Util::ClassName<ZGCache>(),
      m_sequences(cache.m_sequences)
{
}

ZGCache& ZGCache::operator=(const ZGCache &cache)
{
    if (this != &cache)
    {
        m_sequences = cache.m_sequences ;
    }
    return *this ;
}

ZGCache::~ZGCache()
{
}


// Sequence management functions
bool ZGCache::contains(ZInternalID id) const
{
    return (m_sequences.find(id) != m_sequences.end());
}

bool ZGCache::add(ZInternalID id)
{
    if (!id || contains(id))
        return false ;

    m_sequences.insert(ZGCacheElementType(id, ZGSequence(id))) ;

    return true ;
}

std::set<ZInternalID> ZGCache::getSequenceIDs() const
{
    std::set<ZInternalID> dataset;
    ZGCacheContainerType::const_iterator
            it = m_sequences.begin() ;
    for ( ; it!=m_sequences.end() ; ++it)
        dataset.insert(it->first) ;
    return dataset ;
}

const ZGSequence& ZGCache::operator [](ZInternalID id) const
{
    ZGCacheContainerType::const_iterator
            it = m_sequences.find(id) ;
    if (it == m_sequences.end())
        throw std::out_of_range(std::string("ZGCache::operator[]() const ; id not present in container ")) ;
    return it->second ;
}

ZGSequence& ZGCache::operator[](ZInternalID id)
{
    ZGCacheContainerType::iterator
            it = m_sequences.find(id) ;
    if (it == m_sequences.end() )
        throw std::out_of_range(std::string("ZGCache::operator[]() ; id not present in container ")) ;
    return it->second ;
}

bool ZGCache::remove(ZInternalID id)
{
    ZGCacheContainerType::iterator
            it = m_sequences.find(id) ;
    if (it != m_sequences.end())
    {
        m_sequences.erase(it) ;
        return true ;
    }
    return false ;
}

size_t ZGCache::size() const
{
    return m_sequences.size() ;
}

ZGCache::iterator ZGCache::find(ZInternalID id)
{
    return ZGCache::iterator(this, m_sequences.find(id)) ;
}

ZGCache::const_iterator ZGCache::find(ZInternalID id) const
{
    return ZGCache::const_iterator(this, m_sequences.find(id)) ;
}

ZGCache::iterator ZGCache::begin()
{
    return ZGCache::iterator(this, m_sequences.begin() ) ;
}

ZGCache::iterator ZGCache::end()
{
    return ZGCache::iterator(this, m_sequences.end() ) ;
}

ZGCache::const_iterator ZGCache::begin() const
{
    return ZGCache::const_iterator(this, m_sequences.begin() ) ;
}

ZGCache::const_iterator ZGCache::end() const
{
    return ZGCache::const_iterator(this, m_sequences.end() ) ;
}



////////////////////////////////////////////////////////////////////////////////
/// Nested iterator class
////////////////////////////////////////////////////////////////////////////////
ZGCache::iterator::iterator()
    : m_container(nullptr),
      m_iterator()
{
}
ZGCache::iterator::iterator(ZGCache *container, const ZGCacheContainerType::iterator &it)
    : m_container(container),
      m_iterator(it)
{
    if (!m_container)
        throw std::runtime_error(std::string("ZGCache::iterator::iterator() ; container pointer may not be NULL ")) ;
}
ZGCache::iterator::iterator(const ZGCache::iterator& it)
    : m_container(it.m_container),
      m_iterator(it.m_iterator)
{
}
ZGCache::iterator& ZGCache::iterator::operator =(const ZGCache::iterator& it)
{
    if (this != &it)
    {
        m_container = it.m_container ;
        m_iterator = it.m_iterator ;
    }
    return *this ;
}
ZGCache::iterator::~iterator()
{
}
bool ZGCache::iterator::operator==(const iterator& it)
{
    if ((m_container == it.m_container) && (m_iterator==it.m_iterator))
        return true ;
    else
        return false ;
}
bool ZGCache::iterator::operator!=(const iterator& it)
{
    return !operator==(it) ;
}
ZGCache::iterator& ZGCache::iterator::operator ++()
{
    ++m_iterator ;
    return *this ;
}
ZGCache::iterator ZGCache::iterator::operator++(int)
{
    ZGCache::iterator temp(*this) ;
    ++m_iterator ;
    return temp ;
}
ZGSequence& ZGCache::iterator::operator*() const
{
    if (std::distance(m_iterator, m_container->m_sequences.end() ) <= 0)
        throw std::out_of_range(std::string("ZGCache::iterator::operator() ; out of range access")) ;
    return m_iterator->second ;
}
ZGSequence& ZGCache::iterator::sequence() const
{
    if (std::distance(m_iterator, m_container->m_sequences.end() ) <= 0)
        throw std::out_of_range(std::string("ZGCache::iterator::sequence() ; out of range access")) ;
    return m_iterator->second ;
}
ZGSequence* ZGCache::iterator::operator ->() const
{
    if (std::distance(m_iterator, m_container->m_sequences.end() ) <= 0)
        throw std::out_of_range(std::string("ZGCache::iterator::operator->() ; out of range access")) ;
    return &m_iterator->second ;
}

////////////////////////////////////////////////////////////////////////////////
/// Nested const iterator class
////////////////////////////////////////////////////////////////////////////////
ZGCache::const_iterator::const_iterator()
    : m_container(nullptr),
      m_iterator()
{
}
ZGCache::const_iterator::const_iterator(const ZGCache *container, const ZGCacheContainerType::const_iterator &it)
    : m_container(container),
      m_iterator(it)
{
    if (!m_container)
        throw std::runtime_error(std::string("ZGCache::const_iterator::const_iterator() ; container pointer may not be NULL")) ;
}
ZGCache::const_iterator::const_iterator(const ZGCache::const_iterator& it)
    : m_container(it.m_container),
      m_iterator(it.m_iterator)
{
}
ZGCache::const_iterator& ZGCache::const_iterator::operator =(const ZGCache::const_iterator & it)
{
    if (this != &it)
    {
        m_container = it.m_container ;
        m_iterator = it.m_iterator ;
    }
    return *this ;
}
ZGCache::const_iterator::~const_iterator()
{
}

bool ZGCache::const_iterator::operator==(const ZGCache::const_iterator& it)
{
    if ((m_container==it.m_container) && (m_iterator==it.m_iterator))
        return true ;
    else
        return false ;
}
bool ZGCache::const_iterator::operator!=(const ZGCache::const_iterator& it)
{
    return !operator==(it) ;
}
ZGCache::const_iterator& ZGCache::const_iterator::operator++()
{
    ++m_iterator ;
    return *this ;
}
ZGCache::const_iterator ZGCache::const_iterator::operator++(int)
{
    ZGCache::const_iterator temp(*this) ;
    ++m_iterator ;
    return temp ;
}
const ZGSequence& ZGCache::const_iterator::operator*() const
{
    if (std::distance(m_iterator, m_container->m_sequences.end() ) <= 0)
        throw std::out_of_range(std::string("ZGCache::const_iterator::operator() ; out of range access")) ;
    return m_iterator->second ;
}
const ZGSequence& ZGCache::const_iterator::sequence() const
{
    if (std::distance(m_iterator, m_container->m_sequences.end() ) <= 0)
        throw std::out_of_range(std::string("ZGCache::const_iterator::sequence() ; out of range access")) ;
    return m_iterator->second ;
}
const ZGSequence* ZGCache::const_iterator::operator ->() const
{
    if (std::distance(m_iterator, m_container->m_sequences.end() ) <= 0)
        throw std::out_of_range(std::string("ZGCache::const_iterator::operator->() ; out of range access")) ;
    return &m_iterator->second ;
}


} // namespace GUI

} // namespace ZMap
