#include "ZGMessageSetStatusLabel.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageSetStatusLabel>::m_name ("ZGMessageSetStatusLabel") ;
const ZGMessageType ZGMessageSetStatusLabel::m_specific_type (ZGMessageType::SetStatusLabel) ;

ZGMessageSetStatusLabel::ZGMessageSetStatusLabel()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageSetStatusLabel>(),
      Util::ClassName<ZGMessageSetStatusLabel>(),
      m_sequence_id(0),
      m_label(),
      m_tooltip()
{
}

ZGMessageSetStatusLabel::ZGMessageSetStatusLabel(ZInternalID message_id,
                                                 ZInternalID sequence_id,
                                                 const std::string &label,
                                                 const std::string &tooltip)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageSetStatusLabel>(),
      Util::ClassName<ZGMessageSetStatusLabel>(),
      m_sequence_id(sequence_id),
      m_label(label),
      m_tooltip(tooltip)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageSetStatusLabel::ZGMessageSetStatusLabel() ; sequence_id may not be set to 0 ")) ;
}

ZGMessageSetStatusLabel::ZGMessageSetStatusLabel(const ZGMessageSetStatusLabel &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageSetStatusLabel>(),
      Util::ClassName<ZGMessageSetStatusLabel>(),
      m_sequence_id(message.m_sequence_id),
      m_label(message.m_label),
      m_tooltip(message.m_tooltip)
{
}

ZGMessageSetStatusLabel& ZGMessageSetStatusLabel::operator=(const ZGMessageSetStatusLabel& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_label = message.m_label ;
        m_tooltip = message.m_tooltip ;
    }
    return *this ;
}

ZGMessage* ZGMessageSetStatusLabel::clone() const
{
    return new (std::nothrow) ZGMessageSetStatusLabel(*this) ;
}

ZGMessageSetStatusLabel::~ZGMessageSetStatusLabel()
{
}


} // namespace GUI
} // namespace ZMap
