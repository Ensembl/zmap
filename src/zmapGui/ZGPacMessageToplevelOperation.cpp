#include "ZGPacMessageToplevelOperation.h"

#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageToplevelOperation>::m_name("ZGPacMessageToplevelOperation") ;
const ZGPacMessageType ZGPacMessageToplevelOperation::m_specific_type(ZGPacMessageType::ToplevelOperation) ;

ZGPacMessageToplevelOperation::ZGPacMessageToplevelOperation()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageToplevelOperation>(),
      Util::ClassName<ZGPacMessageToplevelOperation>(),
      m_operation_type(Type::Invalid)
{
}

ZGPacMessageToplevelOperation::ZGPacMessageToplevelOperation(ZInternalID message_id,
                                                             ZGPacMessageToplevelOperation::Type type)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageToplevelOperation>(),
      Util::ClassName<ZGPacMessageToplevelOperation>(),
      m_operation_type(type)
{
    if (m_operation_type == Type::Invalid)
        throw std::runtime_error(std::string("ZGPacMessageToplevelOperation::ZGPacMessageToplevelOperation() ; operation type may not be set to Invalid ")) ;
}

ZGPacMessageToplevelOperation::ZGPacMessageToplevelOperation(const ZGPacMessageToplevelOperation &data)
    : ZGPacMessage(data),
      Util::InstanceCounter<ZGPacMessageToplevelOperation>(),
      Util::ClassName<ZGPacMessageToplevelOperation>(),
      m_operation_type(data.m_operation_type)
{
}

ZGPacMessageToplevelOperation& ZGPacMessageToplevelOperation::operator=(const ZGPacMessageToplevelOperation& data)
{
    if (this != &data)
    {
        ZGPacMessage::operator=(data) ;

        m_operation_type = data.m_operation_type ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageToplevelOperation::clone() const
{
    return new (std::nothrow) ZGPacMessageToplevelOperation(*this) ;
}

ZGPacMessageToplevelOperation::~ZGPacMessageToplevelOperation()
{
}


} // namespace GUI

} // namespace ZMap



