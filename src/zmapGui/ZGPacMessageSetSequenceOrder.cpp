#include "ZGPacMessageSetSequenceOrder.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageSetSequenceOrder>::m_name ("ZGPacMessageSetSequenceOrder") ;
const ZGPacMessageType ZGPacMessageSetSequenceOrder::m_specific_type (ZGPacMessageType::SetSequenceOrder) ;

ZGPacMessageSetSequenceOrder::ZGPacMessageSetSequenceOrder()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageSetSequenceOrder>(),
      Util::ClassName<ZGPacMessageSetSequenceOrder>(),
      m_gui_id(0),
      m_sequence_ids()
{
}

ZGPacMessageSetSequenceOrder::ZGPacMessageSetSequenceOrder(ZInternalID message_id,
                                                           ZInternalID gui_id,
                                                           const std::vector<ZInternalID> &data)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageSetSequenceOrder>(),
      Util::ClassName<ZGPacMessageSetSequenceOrder>(),
      m_gui_id(gui_id),
      m_sequence_ids(data)
{
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGPacMessageSetSequenceOrder::ZGPacMessageSetSequenceOrder() ; may not set gui id to 0 ")) ;
    if (!m_sequence_ids.size())
        throw std::runtime_error(std::string("ZGPacMessageSetSequenceOrder::ZGPacMessageSetSequenceOrder() ; container of sequence ids may not be empty")) ;
}

ZGPacMessageSetSequenceOrder::ZGPacMessageSetSequenceOrder(const ZGPacMessageSetSequenceOrder &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageSetSequenceOrder>(),
      Util::ClassName<ZGPacMessageSetSequenceOrder>(),
      m_gui_id(message.m_gui_id),
      m_sequence_ids(message.m_sequence_ids)
{
}

ZGPacMessageSetSequenceOrder& ZGPacMessageSetSequenceOrder::operator=(const ZGPacMessageSetSequenceOrder& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_gui_id = message.m_gui_id ;
        m_sequence_ids = message.m_sequence_ids ;
    }

    return *this ;
}

ZGPacMessage* ZGPacMessageSetSequenceOrder::clone() const
{
    return new (std::nothrow) ZGPacMessageSetSequenceOrder(*this) ;
}

ZGPacMessageSetSequenceOrder::~ZGPacMessageSetSequenceOrder()
{
}

} // namespace GUI

} // namespace ZMap
