#include "ZGMessageScaleBarOperation.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageScaleBarOperation>::m_name("ZGMessageScaleBarOperation") ;
const ZGMessageType ZGMessageScaleBarOperation::m_specific_type(ZGMessageType::ScaleBarOperation) ;

ZGMessageScaleBarOperation::ZGMessageScaleBarOperation()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageScaleBarOperation>(),
      Util::ClassName<ZGMessageScaleBarOperation>(),
      m_sequence_id(0),
      m_gui_id(0),
      m_operation_type(Type::Invalid)
{
}

ZGMessageScaleBarOperation::ZGMessageScaleBarOperation(ZInternalID message_id,
                                                       ZInternalID sequence_id,
                                                       ZInternalID gui_id,
                                                       Type operation_type)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageScaleBarOperation>(),
      Util::ClassName<ZGMessageScaleBarOperation>(),
      m_sequence_id(sequence_id),
      m_gui_id(gui_id),
      m_operation_type(operation_type)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageScaleBarOperation::ZGMessageScaleBarOperation() ; sequence_id may not be set to 0 ")) ;
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGMessageScaleBarOperation::ZGMessageScaleBarOperation() ; gui_id may not be set to 0 ")) ;
    if (m_operation_type == Type::Invalid)
        throw std::runtime_error(std::string("ZGMessageScaleBarOperation::ZGMessageScaleBarOperation() ; operation_type may not be set to Invalid ")) ;
}

ZGMessageScaleBarOperation::ZGMessageScaleBarOperation(const ZGMessageScaleBarOperation &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageScaleBarOperation>(),
      Util::ClassName<ZGMessageScaleBarOperation>(),
      m_sequence_id(message.m_sequence_id),
      m_gui_id(message.m_gui_id),
      m_operation_type(message.m_operation_type)
{
}

ZGMessageScaleBarOperation& ZGMessageScaleBarOperation::operator=(const ZGMessageScaleBarOperation& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_gui_id = message.m_gui_id ;
        m_operation_type = message.m_operation_type ;
    }
    return *this ;
}

ZGMessage* ZGMessageScaleBarOperation::clone() const
{
    return new (std::nothrow) ZGMessageScaleBarOperation(*this) ;
}

ZGMessageScaleBarOperation::~ZGMessageScaleBarOperation()
{
}


} // namespace GUI

} // namespace ZMap

