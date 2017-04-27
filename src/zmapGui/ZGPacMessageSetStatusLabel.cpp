#include "ZGPacMessageSetStatusLabel.h"

#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageSetStatusLabel>::m_name("ZGPacMessageSetStatusLabel") ;
const ZGPacMessageType ZGPacMessageSetStatusLabel::m_specific_type(ZGPacMessageType::SetStatusLabel) ;

ZGPacMessageSetStatusLabel::ZGPacMessageSetStatusLabel()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageSetStatusLabel>(),
      Util::ClassName<ZGPacMessageSetStatusLabel>(),
      m_sequence_id(0),
      m_label()
{
}

ZGPacMessageSetStatusLabel::ZGPacMessageSetStatusLabel(ZInternalID message_id,
                                                       ZInternalID sequence_id,
                                                       const std::string &label,
                                                       const std::string &tooltip )
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageSetStatusLabel>(),
      Util::ClassName<ZGPacMessageSetStatusLabel>(),
      m_sequence_id(sequence_id),
      m_label(label),
      m_tooltip(tooltip)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageSetStatusLabel::ZGPacMessageSetStatusLabel() ; may not set sequence_id to 0 ")) ;
}

ZGPacMessageSetStatusLabel::ZGPacMessageSetStatusLabel(const ZGPacMessageSetStatusLabel &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageSetStatusLabel>(),
      Util::ClassName<ZGPacMessageSetStatusLabel>(),
      m_sequence_id(message.m_sequence_id),
      m_label(message.m_label),
      m_tooltip(message.m_tooltip)
{
}

ZGPacMessageSetStatusLabel& ZGPacMessageSetStatusLabel::operator=(const ZGPacMessageSetStatusLabel& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_label = message.m_label ;
        m_tooltip = message.m_tooltip ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageSetStatusLabel::clone() const
{
    return new (std::nothrow) ZGPacMessageSetStatusLabel(*this) ;
}

ZGPacMessageSetStatusLabel::~ZGPacMessageSetStatusLabel()
{
}

} // namespace GUI
} // namespace ZMap
