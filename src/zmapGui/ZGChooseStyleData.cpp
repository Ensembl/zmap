#include "ZGChooseStyleData.h"
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGChooseStyleData>::m_name ("ZGChooseStyleData") ;

ZGChooseStyleData::ZGChooseStyleData()
    : Util::InstanceCounter<ZGChooseStyleData>(),
      Util::ClassName<ZGChooseStyleData>(),
      m_data()
{
}

ZGChooseStyleData::ZGChooseStyleData(const ZGChooseStyleData &data)
    : Util::InstanceCounter<ZGChooseStyleData>(),
      Util::ClassName<ZGChooseStyleData>(),
      m_data(data.m_data)
{
}

ZGChooseStyleData& ZGChooseStyleData::operator=(const ZGChooseStyleData& data)
{
    if (this != &data)
    {
        m_data = data.m_data ;
    }
    return *this ;
}

ZGChooseStyleData::~ZGChooseStyleData()
{
}

void ZGChooseStyleData::clear()
{
    m_data.clear() ;
}

bool ZGChooseStyleData::isPresent(ZInternalID id) const
{
    return (m_data.find(ZGStyleData(id)) != m_data.end()) ;
}

bool ZGChooseStyleData::isPresent(const ZGStyleData& data) const
{
    return (m_data.find(data) != m_data.end()) ;
}

bool ZGChooseStyleData::addStyleData(const ZGStyleData& data)
{
    if (isPresent(data))
        return false ;
    m_data.insert(data) ;
    return true ;
}

bool ZGChooseStyleData::removeStyleData(const ZInternalID& id)
{
    ZGChooseStyleData_iterator it = m_data.find(ZGStyleData(id)) ;
    if (it != m_data.end())
    {
        m_data.erase(it) ;
        return true ;
    }
    return false ;
}

ZGStyleData ZGChooseStyleData::getStyleData(ZInternalID id) const
{
    ZGChooseStyleData_const_iterator it = m_data.find(ZGStyleData(id)) ;
    if (it != m_data.end())
        return *it ;
    else
        throw std::out_of_range(std::string("ZGChooseStyleData::getStyleData() ; item is not present in container ")) ;
}

std::set<ZInternalID> ZGChooseStyleData::getIDs() const
{
    std::set<ZInternalID> data ;
    ZGChooseStyleData_const_iterator it = m_data.begin() ;
    for ( ; it != m_data.end() ; ++it)
        data.insert(it->getID()) ;
    return data ;
}

void ZGChooseStyleData::merge(const ZGChooseStyleData & data)
{
    std::set<ZInternalID> ids = data.getIDs() ;
    for (auto it = ids.begin() ; it != ids.end() ; ++it)
        m_data.insert(data.getStyleData(*it)) ;
}


} // namespace GUI

} // namespace ZMap


