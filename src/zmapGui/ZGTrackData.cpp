#include "ZGTrackData.h"
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGTrackData>::m_name("ZGTrackData") ;

#ifndef QT_NO_DEBUG
#include <QDebug>
QDebug operator<<(QDebug dbg, const ZGTrackData& data)
{
    dbg.nospace() << QString::fromStdString(data.name())
                  << "(" << data.getID() << ","
                  << QString::fromStdString(data.getTrackName()) << ","
                  << static_cast<unsigned int>(data.getShowForward()) << ","
                  << static_cast<unsigned int>(data.getShowReverse()) << ","
                  << static_cast<unsigned int>(data.getLoad()) << ","
                  << QString::fromStdString(data.getGroup()) << ","
                  << QString::fromStdString(data.getStyles())
                  << ")" ;
    return dbg.space() ;
}
#endif

// This is necessary in order to use ZGTrackData as QVariant metadata type, since
// if conversion is not possible a default constructed instance is returned. I'd
// rather it didn't exist, however.
ZGTrackData::ZGTrackData()
    : Util::InstanceCounter<ZGTrackData>(),
      Util::ClassName<ZGTrackData>(),
      m_id(),
      m_track_name(),
      m_is_current(),
      m_show_forward(ShowType::Invalid),
      m_show_reverse(ShowType::Invalid),
      m_load_type(LoadType::Invalid),
      m_group(),
      m_styles()
{
}

ZGTrackData::ZGTrackData(ZInternalID id)
    : Util::InstanceCounter<ZGTrackData>(),
      Util::ClassName<ZGTrackData>(),
      m_id(id),
      m_track_name(),
      m_is_current(false),
      m_show_forward(ShowType::Show),
      m_show_reverse(ShowType::Show),
      m_load_type(LoadType::All),
      m_group(),
      m_styles()
{
    if (!m_id)
        throw std::runtime_error(std::string("ZGTrackData::ZGTrackData() ; id may not be set to zero ")) ;
}

ZGTrackData::ZGTrackData(ZInternalID id,
                         const std::string& name,
                         ShowType show_forward,
                         ShowType show_reverse,
                         LoadType load,
                         const std::string& group,
                         const std::string& styles)
    : Util::InstanceCounter<ZGTrackData>(),
      Util::ClassName<ZGTrackData>(),
      m_id(id),
      m_track_name(name),
      m_is_current(false),
      m_show_forward(show_forward),
      m_show_reverse(show_reverse),
      m_load_type(load),
      m_group(group),
      m_styles(styles)
{
    if (!m_id)
        throw std::runtime_error(std::string("ZGTrackData::ZGTrackData() ; id may not be set to zero ")) ;
}

ZGTrackData::ZGTrackData(const ZGTrackData& data)
    : Util::InstanceCounter<ZGTrackData>(),
      Util::ClassName<ZGTrackData>(),
      m_id(data.m_id),
      m_track_name(data.m_track_name),
      m_is_current(data.m_is_current),
      m_show_forward(data.m_show_forward),
      m_show_reverse(data.m_show_reverse),
      m_load_type(data.m_load_type),
      m_group(data.m_group),
      m_styles(data.m_styles)
{
    if (!m_id)
        throw std::runtime_error(std::string("ZGTrackData::ZGTrackData() ; id may not be set to zero ")) ;
}

ZGTrackData& ZGTrackData::operator =(const ZGTrackData& data)
{
    if (this != &data)
    {
        m_id = data.m_id ;
        m_track_name = data.m_track_name ;
        m_is_current = data.m_is_current ;
        m_show_forward = data.m_show_forward ;
        m_show_reverse = data.m_show_reverse ;
        m_load_type = data.m_load_type ;
        m_group = data.m_group ;
        m_styles = data.m_styles ;
    }
    return *this ;
}


ZGTrackData::~ZGTrackData()
{
}


bool ZGTrackData::setID(ZInternalID id)
{
    bool result = false ;

    if (!id)
        return result ;

    m_id = id ;
    result = true ;

    return result ;
}

void ZGTrackData::setTrackName(const std::string& name)
{
    m_track_name = name ;
}



void ZGTrackData::setShowForward(ShowType show)
{
    m_show_forward = show ;
}

void ZGTrackData::setShowReverse(ShowType show)
{
    m_show_reverse = show ;
}

void ZGTrackData::setLoad(LoadType load)
{
    m_load_type = load ;
}


void ZGTrackData::setGroup(const std::string &group)
{
    m_group = group ;
}

void ZGTrackData::setStyles(const std::string& styles)
{
    m_styles = styles ;
}

} // namespace GUI

} // namespace ZMap

