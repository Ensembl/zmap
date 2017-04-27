/*  File: ZGFeaturesetSet.cpp
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

#include "ZGFeaturesetSet.h"
#include <stdexcept>
#include <iostream>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGFeaturesetSet>::m_name ("ZGFeaturesetSet") ;

// second version of featureset using stl set instead of map
ZGFeaturesetSet::ZGFeaturesetSet()
    : Util::InstanceCounter<ZGFeaturesetSet>(),
      Util::ClassName<ZGFeaturesetSet>(),
      m_id(0),
      m_features(),
      m_style()
{
}

ZGFeaturesetSet::ZGFeaturesetSet(ZInternalID id)
    : Util::InstanceCounter<ZGFeaturesetSet>(),
      Util::ClassName<ZGFeaturesetSet>(),
      m_id(id),
      m_features(),
      m_style()
{

}

ZGFeaturesetSet::ZGFeaturesetSet(const ZGFeaturesetSet& featureset)
    : Util::InstanceCounter<ZGFeaturesetSet>(),
      Util::ClassName<ZGFeaturesetSet>(),
      m_id(featureset.m_id),
      m_features(featureset.m_features),
      m_style(featureset.m_style)
{
}

ZGFeaturesetSet& ZGFeaturesetSet::operator=(const ZGFeaturesetSet& featureset)
{
    if (this != &featureset)
    {
        m_id = featureset.m_id ;
        m_features = featureset.m_features ;
        m_style = featureset.m_style ;
    }
    return *this ;
}

ZGFeaturesetSet::~ZGFeaturesetSet()
{
}

// this does id-based lookup
const ZGFeature& ZGFeaturesetSet::operator[](ZInternalID id) const
{
    ZGFeaturesetSetContainerType::const_iterator
            it = m_features.find(ZGFeature(id)) ;
    if (it == m_features.end())
        throw std::out_of_range("ZGFeaturesetSet::operator[]; id not present in container ") ;
    return *it ;
}

// fetch all of the ids of features in the container
std::set<ZInternalID> ZGFeaturesetSet::getFeatureIDs() const
{
    std::set<ZInternalID> dataset ;
    for (ZGFeaturesetSetContainerType::const_iterator it = m_features.begin(); it!=m_features.end() ; ++it)
        dataset.insert(it->getID()) ;
    return dataset;
}

bool ZGFeaturesetSet::containsFeature(const ZGFeature &feature) const
{
    return (m_features.find(feature) != m_features.end()) ;
}

bool ZGFeaturesetSet::containsFeature(ZInternalID id) const
{
    return (m_features.find(ZGFeature(id)) != m_features.end() ) ;
}


bool ZGFeaturesetSet::addFeature(const ZGFeature &feature)
{
    if (containsFeature(feature.getID()))
        return false ;
    m_features.insert(feature) ;
    return true ;
}

bool ZGFeaturesetSet::removeFeature(ZInternalID id)
{
    ZGFeaturesetSetContainerType::iterator
            it = m_features.find(ZGFeature(id)) ;
    if (it == m_features.end())
        return false ;
    m_features.erase(it) ;
    return true ;
}



// return the start iterator of the container
ZGFeaturesetSetReader ZGFeaturesetSet::begin()
{
    return ZGFeaturesetSetReader(*this, m_features.begin() ) ;
}


// return the end iterator of the container
ZGFeaturesetSetReader ZGFeaturesetSet::end()
{
    return ZGFeaturesetSetReader(*this, m_features.end() ) ;
}



// read-only access iterator to the container

template <>
const std::string Util::ClassName<ZGFeaturesetSetReader>::m_name ("ZGFeaturesetSetReader") ;

ZGFeaturesetSetReader::ZGFeaturesetSetReader(ZGFeaturesetSet &featureset, const std::set<ZGFeature, ZGFeatureCompID>::iterator &iterator)
    : Util::InstanceCounter<ZGFeaturesetSetReader>(),
      Util::ClassName<ZGFeaturesetSetReader>(),
      m_featureset(featureset),
      m_iterator(iterator)
{

}

ZGFeaturesetSetReader::ZGFeaturesetSetReader(const ZGFeaturesetSetReader &iterator)
    :
      Util::InstanceCounter<ZGFeaturesetSetReader>(),
      Util::ClassName<ZGFeaturesetSetReader>(),
      m_featureset(iterator.m_featureset),
      m_iterator(iterator.m_iterator)
{

}

ZGFeaturesetSetReader& ZGFeaturesetSetReader::operator=(const ZGFeaturesetSetReader &iterator)
{
    if (this != &iterator )
    {
        m_featureset = iterator.m_featureset ;
        m_iterator = iterator.m_iterator ;
    }
    return *this ;
}



bool ZGFeaturesetSetReader::operator ==(const ZGFeaturesetSetReader& iterator)
{
    if ((&m_featureset == &iterator.m_featureset) && (m_iterator == iterator.m_iterator) )
        return true ;
    else
        return false ;
}

bool ZGFeaturesetSetReader::operator !=(const ZGFeaturesetSetReader& iterator)
{
    return !ZGFeaturesetSetReader::operator ==(iterator) ;
}

ZGFeaturesetSetReader& ZGFeaturesetSetReader::operator++()
{
    ++m_iterator ;
    return *this ;
}

ZGFeaturesetSetReader ZGFeaturesetSetReader::operator++(int)
{
    ZGFeaturesetSetReader iterator(*this) ;
    ++m_iterator ;
    return iterator ;
}

const ZGFeature& ZGFeaturesetSetReader::operator*() const
{
    if (std::distance(m_iterator, m_featureset.m_features.end())  <= 0)
        throw std::out_of_range(std::string("ZGFeaturesetSetReader::operator*() ; out of range access" )) ;
    return *m_iterator;
}

const ZGFeature& ZGFeaturesetSetReader::feature() const
{
    if (std::distance(m_iterator, m_featureset.m_features.end()) <= 0)
        throw std::out_of_range(std::string("ZGFeaturesetSetReader::feature() ; out of range access ")) ;
    return *m_iterator ;
}

} // namespace GUI

} // namespace ZMap
