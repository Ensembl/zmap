#include "ZGPacMessageSetSeparatorColor.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageSetSeparatorColor>::m_name("ZGPacMessageSetSeparatorColor") ;
const ZGPacMessageType ZGPacMessageSetSeparatorColor::m_specific_type(ZGPacMessageType::SetSeparatorColor) ;

ZGPacMessageSetSeparatorColor::ZGPacMessageSetSeparatorColor()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageSetSeparatorColor>(),
      Util::ClassName<ZGPacMessageSetSeparatorColor>(),
      m_sequence_id(0),
      m_color()
{
}

ZGPacMessageSetSeparatorColor::ZGPacMessageSetSeparatorColor(ZInternalID message_id,
                                                             ZInternalID sequence_id,
                                                             const ZGColor &color)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageSetSeparatorColor>(),
Util::ClassName<ZGPacMessageSetSeparatorColor>(),
      m_sequence_id(sequence_id),
      m_color(color)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageSetSeparatorColor::ZGPacMessageSetSeparatorColor() ; may not set sequence_id to 0 ")) ;
}


ZGPacMessageSetSeparatorColor::ZGPacMessageSetSeparatorColor(const ZGPacMessageSetSeparatorColor &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageSetSeparatorColor>(),
      Util::ClassName<ZGPacMessageSetSeparatorColor>(),
      m_sequence_id(message.m_sequence_id),
      m_color(message.m_color)
{
}

ZGPacMessageSetSeparatorColor& ZGPacMessageSetSeparatorColor::operator=(const ZGPacMessageSetSeparatorColor& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_color = message.m_color ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageSetSeparatorColor::clone() const
{
    return new (std::nothrow) ZGPacMessageSetSeparatorColor(*this) ;
}

ZGPacMessageSetSeparatorColor::~ZGPacMessageSetSeparatorColor()
{
}


} // namespace GUI

} // namespace ZMap

