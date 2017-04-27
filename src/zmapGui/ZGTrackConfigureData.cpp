#include "ZGTrackConfigureData.h"
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGTrackConfigureData>::m_name("ZGTrackConfigureData") ;

ZGTrackConfigureData::ZGTrackConfigureData()
    : Util::InstanceCounter<ZGTrackConfigureData>(),
      Util::ClassName<ZGTrackConfigureData>(),
      m_data()
{
}

ZGTrackConfigureData::ZGTrackConfigureData(const ZGTrackConfigureData& data)
    : Util::InstanceCounter<ZGTrackConfigureData>(),
      Util::ClassName<ZGTrackConfigureData>(),
      m_data(data.m_data)
{
}

ZGTrackConfigureData& ZGTrackConfigureData::operator=(const ZGTrackConfigureData& data)
{
    if (this != &data)
    {
        m_data = data.m_data ;
    }
    return *this ;
}

ZGTrackConfigureData::~ZGTrackConfigureData()
{
}

void ZGTrackConfigureData::clear()
{
    m_data.clear() ;
}

bool ZGTrackConfigureData::isPresent(const ZGTrackData &data) const
{
    return (m_data.find(data) != m_data.end()) ;
}


bool ZGTrackConfigureData::isPresent(ZInternalID id) const
{
    return (m_data.find(ZGTrackData(id)) != m_data.end()) ;
}

bool ZGTrackConfigureData::addTrackData(const ZGTrackData &data)
{
    if (isPresent(data))
        return false ;
    m_data.insert(data) ;
    return true ;
}

void ZGTrackConfigureData::merge(const ZGTrackConfigureData &data)
{
    std::set<ZInternalID> ids = data.getIDs() ;
    for ( auto it = ids.begin() ; it != ids.end() ; ++it)
        m_data.insert(data.getTrackData(*it)) ;
}

bool ZGTrackConfigureData::removeTrackData(const ZInternalID &id)
{
    auto it = m_data.find(ZGTrackData(id)) ;
    if (it != m_data.end())
    {
        m_data.erase(it) ;
        return true ;
    }
    return false ;
}

ZGTrackData ZGTrackConfigureData::getTrackData(ZInternalID id) const
{
    auto it = m_data.find(ZGTrackData(id)) ;
    if (it != m_data.end())
        return *it ;
    else
        throw std::out_of_range(std::string("ZGTrackConfigureData::getTrackData() ; track not present ")) ;
}



std::set<ZInternalID> ZGTrackConfigureData::getIDs() const
{
    std::set<ZInternalID> data ;
    for ( auto it = m_data.begin() ; it != m_data.end() ; ++it)
        data.insert(it->getID()) ;
    return data ;
}

} // namespace GUI

} // namespace ZMap

