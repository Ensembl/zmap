#include "ZGPacMessageSeparatorOperation.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageSeparatorOperation>::m_name("ZGPacMessageSeparatorOperation") ;
const ZGPacMessageType ZGPacMessageSeparatorOperation::m_specific_type(ZGPacMessageType::SeparatorOperation) ;

ZGPacMessageSeparatorOperation::ZGPacMessageSeparatorOperation()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageSeparatorOperation>(),
      Util::ClassName<ZGPacMessageSeparatorOperation>(),
      m_sequence_id(0),
      m_operation_type(Type::Invalid)
{
}

ZGPacMessageSeparatorOperation::ZGPacMessageSeparatorOperation(ZInternalID message_id,
                                                               ZInternalID sequence_id,
                                                               Type operation_type)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageSeparatorOperation>(),
      Util::ClassName<ZGPacMessageSeparatorOperation>(),
      m_sequence_id(sequence_id),
      m_operation_type(operation_type)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageSeparatorOperation::ZGPacMessageSeparatorOperation() ; may not set sequence id to 0")) ;
    if (m_operation_type == Type::Invalid)
        throw std::runtime_error(std::string("ZGPacMessageSeparatorOperation::ZGPacMessageSeparatorOperation() ; may not set operation type to Invalid ")) ;
}

ZGPacMessageSeparatorOperation::ZGPacMessageSeparatorOperation(const ZGPacMessageSeparatorOperation &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageSeparatorOperation>(),
      Util::ClassName<ZGPacMessageSeparatorOperation>(),
      m_sequence_id(message.m_sequence_id),
      m_operation_type(message.m_operation_type)
{
}

ZGPacMessageSeparatorOperation& ZGPacMessageSeparatorOperation::operator=(const ZGPacMessageSeparatorOperation& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_operation_type = message.m_operation_type ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageSeparatorOperation::clone() const
{
    return new (std::nothrow) ZGPacMessageSeparatorOperation(*this) ;
}

ZGPacMessageSeparatorOperation::~ZGPacMessageSeparatorOperation()
{
}

} // namespace GUI

} // namespace ZMap

