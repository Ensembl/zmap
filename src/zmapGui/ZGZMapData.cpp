#include "ZGZMapData.h"
#include <sstream>

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

namespace ZMap
{

namespace GUI
{

const std::string ZGZMapData::m_name("ZGZMapData") ;

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGZMapData& data)
{
    std::stringstream str ;
    str << data.name() << "(" << data.getID() << "," << data.getState() << ","
        << data.getSequence() << ")" ;
    dbg.nospace() << QString::fromStdString(str.str()) ;
    return dbg.space() ;
}
#endif

ZGZMapData::ZGZMapData()
    : Util::InstanceCounter<ZGZMapData>(),
      m_zmap_id(),
      m_state(),
      m_sequence()
{
}

ZGZMapData::ZGZMapData(const ZGZMapData& data)
    : Util::InstanceCounter<ZGZMapData>(),
      m_zmap_id(data.m_zmap_id),
      m_state(data.m_state),
      m_sequence(data.m_sequence)
{
}


ZGZMapData& ZGZMapData::operator=(const ZGZMapData& data)
{
    if (this != &data)
    {
        m_zmap_id = data.m_zmap_id ;
        m_state = data.m_state ;
        m_sequence = data.m_sequence ;
    }
    return *this ;
}

ZGZMapData::~ZGZMapData()
{
}

bool ZGZMapData::setID(ZInternalID id)
{
    bool result = false ;
    if (id)
    {
        m_zmap_id = id ;
        result = true ;
    }
    return result ;
}

bool ZGZMapData::setState(const std::string& data)
{
    m_state = data ;
    return true ;
}

bool ZGZMapData::setSequence(const std::string& data)
{
    m_sequence = data ;
    return true ;
}

} // namespace GUI

} // namespace ZMap

