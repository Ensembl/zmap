#include "ZGPacMessageSetTrackBGColor.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageSetTrackBGColor>::m_name("ZGPacMessageSetTrackBGColor") ;
const ZGPacMessageType ZGPacMessageSetTrackBGColor::m_specific_type(ZGPacMessageType::SetTrackBGColor) ;

ZGPacMessageSetTrackBGColor::ZGPacMessageSetTrackBGColor()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageSetTrackBGColor>(),
      Util::ClassName<ZGPacMessageSetTrackBGColor>(),
      m_sequence_id(0),
      m_track_id(0),
      m_color()
{
}

ZGPacMessageSetTrackBGColor::ZGPacMessageSetTrackBGColor(ZInternalID message_id,
                                                         ZInternalID sequence_id,
                                                         ZInternalID track_id,
                                                         const ZGColor &color)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageSetTrackBGColor>(),
      Util::ClassName<ZGPacMessageSetTrackBGColor>(),
      m_sequence_id(sequence_id),
      m_track_id(track_id),
      m_color(color)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageSetTrackBGColor::ZGPacMessageSetTrackBGColor() ; may not set sequence_id to 0 ")) ;
    if (!m_track_id)
        throw std::runtime_error(std::string("ZGPacMessageSetTrackBGColor::ZGPacMessageSetTrackBGColor() ; may not set track_id to 0 ")) ;
}

ZGPacMessageSetTrackBGColor::ZGPacMessageSetTrackBGColor(const ZGPacMessageSetTrackBGColor &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageSetTrackBGColor>(),
      Util::ClassName<ZGPacMessageSetTrackBGColor>(),
      m_sequence_id(message.m_sequence_id),
      m_track_id(message.m_track_id),
      m_color(message.m_color)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageSetTrackBGColor::ZGPacMessageSetTrackBGColor() ; may not set sequence_id to 0 ")) ;
    if (!m_track_id)
        throw std::runtime_error(std::string("ZGPacMessageSetTrackBGColor::ZGPacMessageSetTrackBGColor() ; may not set track_id to 0 ")) ;
}

ZGPacMessageSetTrackBGColor& ZGPacMessageSetTrackBGColor::operator=(const ZGPacMessageSetTrackBGColor& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_track_id = message.m_track_id ;
        m_color = message.m_color ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageSetTrackBGColor::clone() const
{
    return new (std::nothrow) ZGPacMessageSetTrackBGColor(*this) ;
}

ZGPacMessageSetTrackBGColor::~ZGPacMessageSetTrackBGColor()
{
}

} // namespace GUI

} // namespace ZMap

