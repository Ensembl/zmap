#include "ZGListFeaturesData.h"
#include <stdexcept>
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGListFeaturesData>::m_name("ZGListFeaturesData") ;

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGListFeaturesData& data)
{
    std::set<ZInternalID> ids = data.getIDs() ;
    for (auto it = ids.begin() ; it != ids.end() ; ++it)
    {
        dbg.nospace() << data.getFeatureData(*it) ;
    }
    return dbg.space() ;
}
#endif

ZGListFeaturesData::ZGListFeaturesData()
    : Util::InstanceCounter<ZGListFeaturesData>(),
      Util::ClassName<ZGListFeaturesData>(),
      m_data()
{
}

ZGListFeaturesData::ZGListFeaturesData(const ZGListFeaturesData &data)
    : Util::InstanceCounter<ZGListFeaturesData>(),
      Util::ClassName<ZGListFeaturesData>(),
      m_data(data.m_data)
{
}

ZGListFeaturesData& ZGListFeaturesData::operator=(const ZGListFeaturesData& data)
{
    if (this != &data)
    {
        m_data = data.m_data ;
    }
    return *this ;
}

ZGListFeaturesData::~ZGListFeaturesData()
{
}

bool ZGListFeaturesData::isPresent(ZInternalID id)
{
    return (m_data.find(ZGFeatureData(id)) != m_data.end()) ;
}

bool ZGListFeaturesData::isPresent(const ZGFeatureData& data)
{
    return (m_data.find(data) != m_data.end()) ;
}

bool ZGListFeaturesData::addFeatureData(const ZGFeatureData& data)
{
    if (isPresent(data))
        return false ;
    m_data.insert(data) ;
    return true ;
}

bool ZGListFeaturesData::removeFeatureData(ZInternalID id)
{
    auto it = m_data.find(ZGFeatureData(id)) ;
    if (it != m_data.end())
    {
        m_data.erase(it) ;
        return true ;
    }
    return false ;
}

ZGFeatureData ZGListFeaturesData::getFeatureData(ZInternalID id) const
{
    auto it = m_data.find(ZGFeatureData(id)) ;
    if (it != m_data.end())
        return *it ;
    else
        throw std::out_of_range(std::string("ZGListFeaturesData::getFeatureData() ; feature not present in container ")) ;
}

std::set<ZInternalID> ZGListFeaturesData::getIDs() const
{
    std::set<ZInternalID> ids ;
    for (auto it = m_data.begin() ; it != m_data.end() ; ++it)
         ids.insert(it->getID() ) ;
    return ids ;
}

void ZGListFeaturesData::merge(const ZGListFeaturesData& data)
{
    std::set<ZInternalID> ids = data.getIDs() ;
    for (auto it = ids.begin() ; it != ids.end() ; ++it)
        m_data.insert(data.getFeatureData(*it)) ;
}

} // namespace GUI

} // namespace ZMap
