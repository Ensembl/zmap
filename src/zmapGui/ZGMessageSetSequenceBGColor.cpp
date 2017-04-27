#include "ZGMessageSetSequenceBGColor.h"
#include <new>
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageSetSequenceBGColor>::m_name("ZGMessageSetSequenceBGColor") ;
const ZGMessageType ZGMessageSetSequenceBGColor::m_specific_type(ZGMessageType::SetSequenceBGColor) ;

ZGMessageSetSequenceBGColor::ZGMessageSetSequenceBGColor()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageSetSequenceBGColor>(),
      Util::ClassName<ZGMessageSetSequenceBGColor>(),
      m_sequence_id(0),
      m_color()
{
}

ZGMessageSetSequenceBGColor::ZGMessageSetSequenceBGColor(ZInternalID message_id,
                                                         ZInternalID sequence_id,
                                                         const ZGColor &color)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageSetSequenceBGColor>(),
      Util::ClassName<ZGMessageSetSequenceBGColor>(),
      m_sequence_id(sequence_id),
      m_color(color)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageSetSequenceBGColor::ZGMessageSetSequenceBGColor() ; may not set sequence_id to zero")) ;
}

ZGMessageSetSequenceBGColor::ZGMessageSetSequenceBGColor(const ZGMessageSetSequenceBGColor &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageSetSequenceBGColor>(),
      Util::ClassName<ZGMessageSetSequenceBGColor>(),
      m_sequence_id(message.m_sequence_id),
      m_color(message.m_color)
{
}
ZGMessageSetSequenceBGColor& ZGMessageSetSequenceBGColor::operator=(const ZGMessageSetSequenceBGColor& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_color = message.m_color ;
    }
    return *this ;
}

ZGMessage* ZGMessageSetSequenceBGColor::clone() const
{
    return new(std::nothrow) ZGMessageSetSequenceBGColor(*this) ;
}

ZGMessageSetSequenceBGColor::~ZGMessageSetSequenceBGColor()
{
}

} // namespace GUI

} // namespace ZMap
