/*  File: ZGFeaturesetCollection.cpp
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

#include "ZGFeaturesetCollection.h"
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGFeaturesetCollection>::m_name("ZGFeaturesetCollection") ;

ZGFeaturesetCollection::ZGFeaturesetCollection()
    : Util::InstanceCounter<ZGFeaturesetCollection>(),
      Util::ClassName<ZGFeaturesetCollection>(),
      m_featuresets()
{
}

ZGFeaturesetCollection::ZGFeaturesetCollection(const ZGFeaturesetCollection &collection)
    : Util::InstanceCounter<ZGFeaturesetCollection>(),
      Util::ClassName<ZGFeaturesetCollection>(),
      m_featuresets(collection.m_featuresets)
{
}

ZGFeaturesetCollection& ZGFeaturesetCollection::operator=(const ZGFeaturesetCollection& collection)
{
    if (this != &collection)
    {
        m_featuresets = collection.m_featuresets ;
    }
    return *this ;
}

ZGFeaturesetCollection::~ZGFeaturesetCollection()
{
}

bool ZGFeaturesetCollection::contains(ZInternalID id) const
{
    ZGFeaturesetCollectionContainerType::const_iterator
            it =  m_featuresets.find(id) ;
    if (it != m_featuresets.end())
        return true ;
    else
        return false ;
}

bool ZGFeaturesetCollection::remove(ZInternalID id)
{
    ZGFeaturesetCollectionContainerType::iterator
            it = m_featuresets.find(id) ;
    if (it != m_featuresets.end())
    {
        m_featuresets.erase(it) ;
        return true ;
    }
    return false ;
}

// the featureset may be present in the container already
bool ZGFeaturesetCollection::add(const ZGFeatureset& featureset)
{
    bool result = false ;
    ZGFeaturesetCollectionContainerType::iterator
            it = m_featuresets.find(featureset.getID()) ;
    if (it == m_featuresets.end())
    {
        m_featuresets.insert(ZGFeaturesetCollectionElementType(featureset.getID(),featureset)) ;
    }
    else
    {
        it->second.merge(featureset) ;
    }
    result = true ;
    return result ;
}

size_t ZGFeaturesetCollection::size() const
{
    return m_featuresets.size() ;
}

std::set<ZInternalID> ZGFeaturesetCollection::getIDs() const
{
    std::set<ZInternalID> data ;
    ZGFeaturesetCollectionContainerType::const_iterator
            it = m_featuresets.begin() ;
    for ( ; it!=m_featuresets.end() ; ++it )
        data.insert(it->first) ;
    return data ;
}

const ZGFeatureset& ZGFeaturesetCollection::operator [](ZInternalID id ) const
{
    ZGFeaturesetCollectionContainerType::const_iterator
            it = m_featuresets.find(id) ;
    if (it == m_featuresets.end())
        throw std::out_of_range(std::string("ZGFeaturesetCollection::operator[]() const ; id not present in container ")) ;
    return it->second ;
}

ZGFeatureset& ZGFeaturesetCollection::operator[](ZInternalID id)
{
    ZGFeaturesetCollectionContainerType::iterator
            it = m_featuresets.find(id) ;
    if (it == m_featuresets.end())
        throw std::out_of_range(std::string("ZGFeaturesetCollection::operator[]() ; id not present in container ")) ;
    return it->second ;
}


ZGFeaturesetCollection::iterator ZGFeaturesetCollection::find(ZInternalID id)
{
    return ZGFeaturesetCollection::iterator(this, m_featuresets.find(id)) ;
}

ZGFeaturesetCollection::const_iterator ZGFeaturesetCollection::find(ZInternalID id) const
{
    return ZGFeaturesetCollection::const_iterator(this, m_featuresets.find(id)) ;
}

ZGFeaturesetCollection::iterator ZGFeaturesetCollection::begin()
{
    return ZGFeaturesetCollection::iterator(this, m_featuresets.begin() ) ;
}

ZGFeaturesetCollection::iterator ZGFeaturesetCollection::end()
{
    return ZGFeaturesetCollection::iterator(this, m_featuresets.end() ) ;
}

ZGFeaturesetCollection::const_iterator ZGFeaturesetCollection::begin() const
{
    return ZGFeaturesetCollection::const_iterator(this, m_featuresets.begin() ) ;
}

ZGFeaturesetCollection::const_iterator ZGFeaturesetCollection::end() const
{
    return ZGFeaturesetCollection::const_iterator(this, m_featuresets.end() ) ;
}

std::ostream& operator<<(std::ostream& str, const ZGFeaturesetCollection& featureset_collection)
{
    for (auto it=featureset_collection.begin() ; it!=featureset_collection.end() ; ++it)
    {
        str << it->getID() << " " ;
    }
    return str ;
}



////////////////////////////////////////////////////////////////////////////////
/// iterator access
////////////////////////////////////////////////////////////////////////////////
ZGFeaturesetCollection::iterator::iterator()
    : m_container(nullptr),
      m_iterator()
{
}
ZGFeaturesetCollection::iterator::iterator(ZGFeaturesetCollection *container, const ZGFeaturesetCollectionContainerType::iterator &it)
    : m_container(container),
      m_iterator(it)
{
    if (!m_container)
        throw std::runtime_error(std::string("ZGFeaturesetCollection::iterator::iterator() ; container pointer may not be NULL ")) ;
}
ZGFeaturesetCollection::iterator::iterator(const ZGFeaturesetCollection::iterator& it)
    : m_container(it.m_container),
      m_iterator(it.m_iterator)
{
}
ZGFeaturesetCollection::iterator& ZGFeaturesetCollection::iterator::operator =(const ZGFeaturesetCollection::iterator& it)
{
    if (this != &it)
    {
        m_container = it.m_container ;
        m_iterator = it.m_iterator ;
    }
    return *this ;
}
ZGFeaturesetCollection::iterator::~iterator()
{
}

bool ZGFeaturesetCollection::iterator::operator==(const iterator& it)
{
    if ((m_container == it.m_container) && (m_iterator==it.m_iterator))
        return true ;
    else
        return false ;
}
bool ZGFeaturesetCollection::iterator::operator!=(const iterator& it)
{
    return !operator==(it) ;
}
ZGFeaturesetCollection::iterator& ZGFeaturesetCollection::iterator::operator ++()
{
    ++m_iterator ;
    return *this ;
}
ZGFeaturesetCollection::iterator ZGFeaturesetCollection::iterator::operator++(int)
{
    ZGFeaturesetCollection::iterator temp(*this) ;
    ++m_iterator ;
    return temp ;
}
ZGFeatureset& ZGFeaturesetCollection::iterator::operator*() const
{
    if (std::distance(m_iterator, m_container->m_featuresets.end() ) <= 0)
        throw std::out_of_range(std::string("ZGFeaturesetCollection::iterator::operator() ; out of range access")) ;
    return m_iterator->second ;
}
ZGFeatureset& ZGFeaturesetCollection::iterator::featureset() const
{
    if (std::distance(m_iterator, m_container->m_featuresets.end() ) <= 0)
        throw std::out_of_range(std::string("ZGFeaturesetCollection::iterator::sequence() ; out of range access")) ;
    return m_iterator->second ;
}
ZGFeatureset* ZGFeaturesetCollection::iterator::operator ->() const
{
    if (std::distance(m_iterator, m_container->m_featuresets.end() ) <= 0)
        throw std::out_of_range(std::string("ZGFeaturesetCollection::iterator::operator->() ; out of range access")) ;
    return &m_iterator->second ;
}




////////////////////////////////////////////////////////////////////////////////
/// const_iterator access
////////////////////////////////////////////////////////////////////////////////


ZGFeaturesetCollection::const_iterator::const_iterator()
    : m_container(nullptr),
      m_iterator()
{
}
ZGFeaturesetCollection::const_iterator::const_iterator(const ZGFeaturesetCollection *container, const ZGFeaturesetCollectionContainerType::const_iterator &it)
    : m_container(container),
      m_iterator(it)
{
    if (!m_container)
        throw std::runtime_error(std::string("ZGFeaturesetCollection::const_iterator::const_iterator() ; container pointer may not be NULL")) ;
}
ZGFeaturesetCollection::const_iterator::const_iterator(const ZGFeaturesetCollection::const_iterator& it)
    : m_container(it.m_container),
      m_iterator(it.m_iterator)
{
}
ZGFeaturesetCollection::const_iterator& ZGFeaturesetCollection::const_iterator::operator =(const ZGFeaturesetCollection::const_iterator & it)
{
    if (this != &it)
    {
        m_container = it.m_container ;
        m_iterator = it.m_iterator ;
    }
    return *this ;
}
ZGFeaturesetCollection::const_iterator::~const_iterator()
{
}

bool ZGFeaturesetCollection::const_iterator::operator==(const ZGFeaturesetCollection::const_iterator& it)
{
    if ((m_container==it.m_container) && (m_iterator==it.m_iterator))
        return true ;
    else
        return false ;
}
bool ZGFeaturesetCollection::const_iterator::operator!=(const ZGFeaturesetCollection::const_iterator& it)
{
    return !operator==(it) ;
}
ZGFeaturesetCollection::const_iterator& ZGFeaturesetCollection::const_iterator::operator++()
{
    ++m_iterator ;
    return *this ;
}
ZGFeaturesetCollection::const_iterator ZGFeaturesetCollection::const_iterator::operator++(int)
{
    ZGFeaturesetCollection::const_iterator temp(*this) ;
    ++m_iterator ;
    return temp ;
}
const ZGFeatureset& ZGFeaturesetCollection::const_iterator::operator*() const
{
    if (std::distance(m_iterator, m_container->m_featuresets.end() ) <= 0)
        throw std::out_of_range(std::string("ZGFeaturesetCollection::const_iterator::operator() ; out of range access")) ;
    return m_iterator->second ;
}
const ZGFeatureset& ZGFeaturesetCollection::const_iterator::featureset() const
{
    if (std::distance(m_iterator, m_container->m_featuresets.end() ) <= 0)
        throw std::out_of_range(std::string("ZGFeaturesetCollection::const_iterator::sequence() ; out of range access")) ;
    return m_iterator->second ;
}
const ZGFeatureset* ZGFeaturesetCollection::const_iterator::operator ->() const
{
    if (std::distance(m_iterator, m_container->m_featuresets.end() ) <= 0)
        throw std::out_of_range(std::string("ZGFeaturesetCollection::const_iterator::operator->() ; out of range access")) ;
    return &m_iterator->second ;
}


} // namespace GUI

} // namespace ZMap

