#include "ZGMessageToplevelOperation.h"

#include <stdexcept>
#include <new>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageToplevelOperation>::m_name("ZGMessageToplevelOperation") ;
const ZGMessageType ZGMessageToplevelOperation::m_specific_type(ZGMessageType::ToplevelOperation) ;

ZGMessageToplevelOperation::ZGMessageToplevelOperation()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageToplevelOperation>(),
      Util::ClassName<ZGMessageToplevelOperation>(),
      m_operation_type(Type::Invalid)
{
}

ZGMessageToplevelOperation::ZGMessageToplevelOperation(ZInternalID message_id,
                                                       ZGMessageToplevelOperation::Type type)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageToplevelOperation>(),
      Util::ClassName<ZGMessageToplevelOperation>(),
      m_operation_type(type)
{
    if (m_operation_type == Type::Invalid)
        throw std::runtime_error(std::string("ZGMessageToplevelOperation::ZGMessageToplevelOperation() ; operation_type may not be set to Invalid ")) ;
}

ZGMessageToplevelOperation::ZGMessageToplevelOperation(const ZGMessageToplevelOperation &data)
    : ZGMessage(data),
      Util::InstanceCounter<ZGMessageToplevelOperation>(),
      Util::ClassName<ZGMessageToplevelOperation>(),
      m_operation_type(data.m_operation_type)
{
}

ZGMessageToplevelOperation& ZGMessageToplevelOperation::operator=(const ZGMessageToplevelOperation& data)
{
    if (this != &data)
    {
        ZGMessage::operator=(data) ;

        m_operation_type = data.m_operation_type ;
    }
    return *this ;
}

ZGMessage* ZGMessageToplevelOperation::clone() const
{
    return new (std::nothrow) ZGMessageToplevelOperation(*this) ;
}

ZGMessageToplevelOperation::~ZGMessageToplevelOperation()
{
}

} // namespace GUI

} // namespace ZMap

