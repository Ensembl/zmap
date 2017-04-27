#include "ZGMessageSetSequenceOrder.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageSetSequenceOrder>::m_name("ZGMessageSetSequenceOrder") ;
const ZGMessageType ZGMessageSetSequenceOrder::m_specific_type(ZGMessageType::SetSequenceOrder) ;

ZGMessageSetSequenceOrder::ZGMessageSetSequenceOrder()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageSetSequenceOrder>(),
      Util::ClassName<ZGMessageSetSequenceOrder>(),
      m_gui_id(0),
      m_sequence_ids()
{
}

ZGMessageSetSequenceOrder::ZGMessageSetSequenceOrder(ZInternalID message_id,
                                                     ZInternalID gui_id,
                                                     const std::vector<ZInternalID> &data)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageSetSequenceOrder>(),
      Util::ClassName<ZGMessageSetSequenceOrder>(),
      m_gui_id(gui_id),
      m_sequence_ids(data)
{
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGMessageSetSequenceOrder::ZGMessageSetSequenceOrder()  ; may not set gui_id to 0 ")) ;
    if (!m_sequence_ids.size())
        throw std::runtime_error(std::string("ZGMessageSetSequenceOrder::ZGMessageSetSequenceOrder() ; container of sequence_ids may not be empty")) ;
}

ZGMessageSetSequenceOrder::ZGMessageSetSequenceOrder(const ZGMessageSetSequenceOrder& message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageSetSequenceOrder>(),
      Util::ClassName<ZGMessageSetSequenceOrder>(),
      m_gui_id(message.m_gui_id),
      m_sequence_ids(message.m_sequence_ids)
{
}

ZGMessageSetSequenceOrder& ZGMessageSetSequenceOrder::operator=(const ZGMessageSetSequenceOrder& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_gui_id = message.m_gui_id ;
        m_sequence_ids = message.m_sequence_ids ;
    }
    return *this ;
}

ZGMessage* ZGMessageSetSequenceOrder::clone() const
{
    return new (std::nothrow) ZGMessageSetSequenceOrder(*this) ;
}

ZGMessageSetSequenceOrder::~ZGMessageSetSequenceOrder()
{
}

} // namespace GUI

} // namespace ZMap
