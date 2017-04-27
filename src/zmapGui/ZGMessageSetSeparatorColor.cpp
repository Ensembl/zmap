#include "ZGMessageSetSeparatorColor.h"
#include <new>
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageSetSeparatorColor>::m_name ("ZGMessageSetSeparatorColor") ;
const ZGMessageType ZGMessageSetSeparatorColor::m_specific_type(ZGMessageType::SetSeparatorColor) ;

ZGMessageSetSeparatorColor::ZGMessageSetSeparatorColor()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageSetSeparatorColor>(),
      Util::ClassName<ZGMessageSetSeparatorColor>(),
      m_sequence_id(0),
      m_color()
{
}

ZGMessageSetSeparatorColor::ZGMessageSetSeparatorColor(ZInternalID message_id,
                                                       ZInternalID sequence_id,
                                                       const ZGColor &color)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageSetSeparatorColor>(),
      Util::ClassName<ZGMessageSetSeparatorColor>(),
      m_sequence_id(sequence_id),
      m_color(color)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageSetSeparatorColor::ZGMessageSetSeparatorColor() ; sequence_id may not be set to 0 ")) ;
}

ZGMessageSetSeparatorColor::ZGMessageSetSeparatorColor(const ZGMessageSetSeparatorColor &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageSetSeparatorColor>(),
      Util::ClassName<ZGMessageSetSeparatorColor>(),
      m_sequence_id(message.m_sequence_id),
      m_color(message.m_color)
{
}


ZGMessageSetSeparatorColor& ZGMessageSetSeparatorColor::operator=(const ZGMessageSetSeparatorColor& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_color = message.m_color ;
    }
    return *this ;
}

ZGMessage * ZGMessageSetSeparatorColor::clone() const
{
    return new (std::nothrow) ZGMessageSetSeparatorColor(*this) ;
}

ZGMessageSetSeparatorColor::~ZGMessageSetSeparatorColor()
{
}

} // namespace GUI

} // namespace ZMap

