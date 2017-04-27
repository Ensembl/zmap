#include "ZGMessageSetTrackBGColor.h"
#include <new>
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageSetTrackBGColor>::m_name("ZGMessageSetTrackBGColor") ;
const ZGMessageType ZGMessageSetTrackBGColor::m_specific_type(ZGMessageType::SetTrackBGColor) ;

ZGMessageSetTrackBGColor::ZGMessageSetTrackBGColor()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageSetTrackBGColor>(),
      Util::ClassName<ZGMessageSetTrackBGColor>(),
      m_sequence_id(0),
      m_track_id(0),
      m_color()
{
}

ZGMessageSetTrackBGColor::ZGMessageSetTrackBGColor(ZInternalID message_id,
                                                   ZInternalID sequence_id,
                                                   ZInternalID track_id,
                                                   const ZGColor &color)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageSetTrackBGColor>(),
      Util::ClassName<ZGMessageSetTrackBGColor>(),
      m_sequence_id(sequence_id),
      m_track_id(track_id),
      m_color(color)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageSetTrackBGColor::ZGMessageSetTrackBGColor() ; sequence_id may not be set to 0 ")) ;
    if (!m_track_id)
        throw std::runtime_error(std::string("ZGMessageSetTrackBGColor::ZGMessageSetTrackBGColor() ; track_id may not be set to 0")) ;
}

ZGMessageSetTrackBGColor::ZGMessageSetTrackBGColor(const ZGMessageSetTrackBGColor &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageSetTrackBGColor>(),
      Util::ClassName<ZGMessageSetTrackBGColor>(),
      m_sequence_id(message.m_sequence_id),
      m_track_id(message.m_track_id),
      m_color(message.m_color)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageSetTrackBGColor::ZGMessageSetTrackBGColor() ; sequence_id may not be set to 0 ")) ;
    if (m_track_id)
        throw std::runtime_error(std::string("ZGMessageSetTrackBGColor::ZGMessageSetTrackBGColor() ; track_id may not be set to 0")) ;
}

ZGMessageSetTrackBGColor& ZGMessageSetTrackBGColor::operator=(const ZGMessageSetTrackBGColor& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_track_id = message.m_track_id ;
        m_color = message.m_color ;
    }
    return *this ;
}

ZGMessage* ZGMessageSetTrackBGColor::clone() const
{
    return new(std::nothrow) ZGMessageSetTrackBGColor(*this) ;
}

ZGMessageSetTrackBGColor::~ZGMessageSetTrackBGColor()
{
}

} // namespace GUI

} // namespace ZMap

