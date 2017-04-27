/*  File: ZGFeatureset.cpp
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

#include "ZGFeatureset.h"
#include <stdexcept>
#include <iostream>


////////////////////////////////////////////////////////////////////////////////
//// featureset container
////////////////////////////////////////////////////////////////////////////////

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGFeatureset>::m_name("ZGFeatureset") ;

ZGFeatureset::ZGFeatureset()
    : Util::InstanceCounter<ZGFeatureset>(),
      Util::ClassName<ZGFeatureset>(),
      m_id(0),
      m_features()
{
}

ZGFeatureset::ZGFeatureset(ZInternalID id)
    : Util::InstanceCounter<ZGFeatureset>(),
      Util::ClassName<ZGFeatureset>(),
      m_id(id),
      m_features()
{
    if (!m_id)
        throw std::runtime_error(std::string("ZGFeatureset::ZGFeatureset() ; id may not be 0 ")) ;
}

ZGFeatureset::ZGFeatureset(const ZGFeatureset& featureset)
    : Util::InstanceCounter<ZGFeatureset>(),
      Util::ClassName<ZGFeatureset>(),
      m_id(featureset.m_id),
      m_features(featureset.m_features)
{
}

ZGFeatureset& ZGFeatureset::operator=(const ZGFeatureset& featureset)
{
    if (this != &featureset)
    {
        m_id = featureset.m_id ;
        m_features = featureset.m_features ;
    }
    return *this ;
}

// return const ref to feature with argument id
const ZGFeature& ZGFeatureset::operator[](ZInternalID id) const
{
    ZGFeaturesetContainerType::const_iterator
            it = m_features.find(id) ;
    if (it == m_features.end())
        throw std::out_of_range("ZGFeatureset::operator[]() const ; id not present in container ") ;
    return it->second ;
}

ZGFeature& ZGFeatureset::operator[](ZInternalID id)
{
    ZGFeaturesetContainerType::iterator
            it = m_features.find(id) ;
    if (it == m_features.end())
        throw std::out_of_range("ZGFeatureset::operator[]; id not present in container ") ;
    return it->second ;
}

ZGFeatureset::~ZGFeatureset()
{
}

bool ZGFeatureset::containsFeature(ZInternalID id) const
{
    return (m_features.find(id) != m_features.end() ) ;
}


bool ZGFeatureset::addFeature(const ZGFeature &feature)
{
    if (containsFeature(feature.getID()))
        return false ;
    m_features.insert(ZGFeaturesetElementType(feature.getID(),feature)) ;
    return true ;
}

bool ZGFeatureset::addFeatures(const ZGFeaturesetContainerType &features)
{
    ZGFeaturesetContainerType::const_iterator it = features.begin() ;
    for ( ; it!=features.end() ; ++it )
        addFeature(it->second) ;
    return true ;
}

bool ZGFeatureset::removeFeature(ZInternalID id)
{
    ZGFeaturesetContainerType::iterator
            it = m_features.find(id) ;
    if (it == m_features.end())
        return false ;
    m_features.erase(it) ;
    return true ;
}

bool ZGFeatureset::removeFeatures(const std::set<ZInternalID> &dataset)
{
    if (!dataset.size())
        return false ;
    for (auto it=dataset.begin() ; it!=dataset.end() ; ++it)
    {
        m_features.erase(*it) ;
    }
    return true ;
}

void ZGFeatureset::removeFeaturesAll()
{
    m_features.clear();
}

// merge the argument featureset into this one; it only really makes sense to
// do this operation if the featureset ids are the same, i.e. we are adding data
// to the featureset from the same source. if a feature with the same id already exists,
// it will _not_ overwrite the element (we are using std::map::insert())
bool ZGFeatureset::merge(const ZGFeatureset &featureset)
{
    if (m_id != featureset.getID() )
        return false ;
    for (ZGFeatureset::const_iterator it=featureset.begin() ; it!=featureset.end() ; ++it)
        m_features.insert(ZGFeaturesetElementType(it.feature().getID(), it.feature())) ;
        //addFeature(it.feature()) ; // equivalently...
    return true ;
}

// replace (copy assign) a feature with the argument
bool ZGFeatureset::replaceFeature(const ZGFeature& feature)
{
    ZGFeaturesetContainerType::iterator it = m_features.find(feature.getID()) ;
    if (it == m_features.end())
        return false ;
    it->second = feature ;
    return true ;
}


unsigned int ZGFeatureset::size() const
{
    return static_cast<unsigned int>(m_features.size()) ;
}

std::set<ZInternalID> ZGFeatureset::getFeatureIDs() const
{
    std::set<ZInternalID> dataset ;
    for (ZGFeaturesetContainerType::const_iterator it = m_features.begin(); it!=m_features.end() ; ++it)
        dataset.insert(it->first) ;
    return dataset ;
}

// begin and end using the nested class iterators
ZGFeatureset::iterator ZGFeatureset::begin()
{
    return iterator(this, m_features.begin()) ;
}
ZGFeatureset::iterator ZGFeatureset::end()
{
    return iterator(this, m_features.end()) ;
}
ZGFeatureset::const_iterator ZGFeatureset::begin() const
{
    return const_iterator(this, m_features.begin()) ;
}
ZGFeatureset::const_iterator ZGFeatureset::end() const
{
    return const_iterator(this, m_features.end()) ;
}
ZGFeatureset::iterator ZGFeatureset::find(ZInternalID id)
{
    return ZGFeatureset::iterator(this, m_features.find(id)) ;
}
ZGFeatureset::const_iterator ZGFeatureset::find(ZInternalID id) const
{
    return ZGFeatureset::const_iterator(this, m_features.find(id)) ;
}


////////////////////////////////////////////////////////////////////////////////
////  iterator
////////////////////////////////////////////////////////////////////////////////
ZGFeatureset::iterator::iterator()
    : m_container(nullptr),
      m_iterator()
{

}
ZGFeatureset::iterator::iterator(ZGFeatureset *container, const ZGFeaturesetContainerType::iterator &it)
    : m_container(container),
      m_iterator(it)
{
    if (!m_container)
        throw std::runtime_error(std::string("ZGFeatureset::iterator::iterator() ; container pointer may not be NULL ")) ;
}
ZGFeatureset::iterator::iterator(const ZGFeatureset::iterator& it)
    : m_container(it.m_container),
      m_iterator(it.m_iterator)
{

}
ZGFeatureset::iterator& ZGFeatureset::iterator::operator=(const ZGFeatureset::iterator& it)
{
    if (this != &it)
    {
        m_container = it.m_container ;
        m_iterator = it.m_iterator ;
    }
    return *this ;
}
bool ZGFeatureset::iterator::operator==(const ZGFeatureset::iterator& rhs) const
{
    if ((m_container==rhs.m_container) && (m_iterator == rhs.m_iterator))
        return true ;
    else
        return false ;
}
bool ZGFeatureset::iterator::operator!=(const ZGFeatureset::iterator& rhs) const
{
    return !ZGFeatureset::iterator::operator==(rhs) ;
}
ZGFeatureset::iterator& ZGFeatureset::iterator::operator++()
{
    ++m_iterator ;
    return *this ;
}
ZGFeatureset::iterator ZGFeatureset::iterator::operator++(int)
{
    ZGFeatureset::iterator temp(*this) ;
    ++m_iterator ;
    return temp ;
}
ZGFeature& ZGFeatureset::iterator::operator*() const
{
    if (std::distance(m_iterator, m_container->m_features.end())  <= 0)
        throw std::out_of_range(std::string("ZGFeatureset::iterator::operator*() ; out of range access" )) ;
    return m_iterator->second ;
}
ZGFeature& ZGFeatureset::iterator::feature() const
{
    if (std::distance(m_iterator, m_container->m_features.end())  <= 0)
        throw std::out_of_range(std::string("ZGFeatureset::iterator::feature() ; out of range access" )) ;
    return m_iterator->second ;
}
ZGFeature* ZGFeatureset::iterator::operator->() const
{
    if (std::distance(m_iterator, m_container->m_features.end())  <= 0)
        throw std::out_of_range(std::string("ZGFeatureset::iterator::operator->() ; out of range access" )) ;
    return &m_iterator->second;
}

////////////////////////////////////////////////////////////////////////////////
/// const iterator
////////////////////////////////////////////////////////////////////////////////
ZGFeatureset::const_iterator::const_iterator()
    : m_container(nullptr),
      m_iterator()
{

}
ZGFeatureset::const_iterator::const_iterator(const ZGFeatureset *container, const ZGFeaturesetContainerType::const_iterator &it)
    : m_container(container),
      m_iterator(it)
{
    if (!m_container)
        throw std::runtime_error(std::string("ZGFeatureset::const_iterator::const_iterator() ; container pointer may not be NULL ")) ;
}
ZGFeatureset::const_iterator::const_iterator(const ZGFeatureset::const_iterator& it)
    : m_container(it.m_container),
      m_iterator(it.m_iterator)
{

}
ZGFeatureset::const_iterator& ZGFeatureset::const_iterator::operator=(const ZGFeatureset::const_iterator& it)
{
    if (this != &it)
    {
        m_container = it.m_container ;
        m_iterator = it.m_iterator ;
    }
    return *this ;
}
bool ZGFeatureset::const_iterator::operator==(const ZGFeatureset::const_iterator& rhs) const
{
    if ((m_container==rhs.m_container) && (m_iterator == rhs.m_iterator))
        return true ;
    else
        return false ;
}
bool ZGFeatureset::const_iterator::operator!=(const ZGFeatureset::const_iterator& rhs) const
{
    return !ZGFeatureset::const_iterator::operator==(rhs) ;
}
ZGFeatureset::const_iterator& ZGFeatureset::const_iterator::operator++()
{
    ++m_iterator ;
    return *this ;
}
ZGFeatureset::const_iterator ZGFeatureset::const_iterator::operator++(int)
{
    ZGFeatureset::const_iterator temp(*this) ;
    ++m_iterator ;
    return temp ;
}
const ZGFeature& ZGFeatureset::const_iterator::operator*() const
{
    if (std::distance(m_iterator, m_container->m_features.end() )  <= 0)
        throw std::out_of_range(std::string("ZGFeatureset::const_iterator::operator*() ; out of range access" )) ;
    return m_iterator->second ;
}
const ZGFeature& ZGFeatureset::const_iterator::feature() const
{
    if (std::distance(m_iterator, m_container->m_features.end() )  <= 0)
        throw std::out_of_range(std::string("ZGFeatureset::const_iterator::operator*() ; out of range access" )) ;
    return m_iterator->second ;
}
const ZGFeature* ZGFeatureset::const_iterator::operator->() const
{
    if (std::distance(m_iterator, m_container->m_features.end() )  <= 0)
        throw std::out_of_range(std::string("ZGFeatureset::const_iterator::operator->() ; out of range access" ) ) ;
    return &m_iterator->second;
}


} // namespace GUI

} // namespace ZMap
