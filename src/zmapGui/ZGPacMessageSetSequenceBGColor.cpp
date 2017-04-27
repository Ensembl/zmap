#include "ZGPacMessageSetSequenceBGColor.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageSetSequenceBGColor>::m_name("ZGPacMessageSetSequenceBGColor") ;
const ZGPacMessageType ZGPacMessageSetSequenceBGColor::m_specific_type(ZGPacMessageType::SetSequenceBGColor) ;

ZGPacMessageSetSequenceBGColor::ZGPacMessageSetSequenceBGColor()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageSetSequenceBGColor>(),
      Util::ClassName<ZGPacMessageSetSequenceBGColor>(),
      m_sequence_id(0),
      m_color()
{
}

ZGPacMessageSetSequenceBGColor::ZGPacMessageSetSequenceBGColor(ZInternalID message_id,
                                                               ZInternalID sequence_id,
                                                               const ZGColor &color)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageSetSequenceBGColor>(),
      Util::ClassName<ZGPacMessageSetSequenceBGColor>(),
      m_sequence_id(sequence_id),
      m_color(color)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageSetSequenceBGColor::ZGPacMessageSetSequenceBGColor() ; may not set sequence id to 0 ")) ;
}

ZGPacMessageSetSequenceBGColor::ZGPacMessageSetSequenceBGColor(const ZGPacMessageSetSequenceBGColor &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageSetSequenceBGColor>(),
      Util::ClassName<ZGPacMessageSetSequenceBGColor>(),
      m_sequence_id(message.m_sequence_id),
      m_color(message.m_color)
{
}

ZGPacMessageSetSequenceBGColor& ZGPacMessageSetSequenceBGColor::operator=(const ZGPacMessageSetSequenceBGColor& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_color = message.m_color ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageSetSequenceBGColor::clone() const
{
    return new (std::nothrow) ZGPacMessageSetSequenceBGColor(*this) ;
}

ZGPacMessageSetSequenceBGColor::~ZGPacMessageSetSequenceBGColor()
{
}

} // namespace GUI

} // namespace ZMap
