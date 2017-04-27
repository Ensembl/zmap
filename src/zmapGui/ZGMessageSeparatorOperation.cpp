#include "ZGMessageSeparatorOperation.h"
#include <new>
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageSeparatorOperation>::m_name ("ZGMessageSeparatorOperation") ;
const ZGMessageType ZGMessageSeparatorOperation::m_specific_type(ZGMessageType::SeparatorOperation) ;

ZGMessageSeparatorOperation::ZGMessageSeparatorOperation()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageSeparatorOperation>(),
      Util::ClassName<ZGMessageSeparatorOperation>(),
      m_sequence_id(0),
      m_operation_type(Type::Invalid)
{
}

ZGMessageSeparatorOperation::ZGMessageSeparatorOperation(ZInternalID message_id,
                                                         ZInternalID sequence_id,
                                                         Type operation_type)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageSeparatorOperation>(),
      Util::ClassName<ZGMessageSeparatorOperation>(),
      m_sequence_id(sequence_id),
      m_operation_type(operation_type)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageSeparatorOperation::ZGMessageSeparatorOperation() ; may not set sequence_id to 0 ")) ;
    if (m_operation_type == Type::Invalid)
        throw std::runtime_error(std::string("ZGMessageSeparatorOperation::ZGMessageSeparatorOperation() ; may not set operation_type to Invalid ")) ;
}

ZGMessageSeparatorOperation::ZGMessageSeparatorOperation(const ZGMessageSeparatorOperation &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageSeparatorOperation>(),
      Util::ClassName<ZGMessageSeparatorOperation>(),
      m_sequence_id(message.m_sequence_id),
      m_operation_type(message.m_operation_type)
{
}

ZGMessageSeparatorOperation& ZGMessageSeparatorOperation::operator=(const ZGMessageSeparatorOperation& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_operation_type = message.m_operation_type ;
    }
    return *this ;
}

ZGMessage* ZGMessageSeparatorOperation::clone() const
{
    return new (std::nothrow) ZGMessageSeparatorOperation(*this) ;
}

ZGMessageSeparatorOperation::~ZGMessageSeparatorOperation()
{
}

} // namespace GUI

} // namespace ZMap

