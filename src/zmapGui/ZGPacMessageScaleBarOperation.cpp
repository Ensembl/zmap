#include "ZGPacMessageScaleBarOperation.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageScaleBarOperation>::m_name ("ZGPacMessageScaleBarOperation") ;
const ZGPacMessageType ZGPacMessageScaleBarOperation::m_specific_type(ZGPacMessageType::ScaleBarOperation) ;

ZGPacMessageScaleBarOperation::ZGPacMessageScaleBarOperation()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageScaleBarOperation>(),
      Util::ClassName<ZGPacMessageScaleBarOperation>(),
      m_sequence_id(0),
      m_gui_id(0),
      m_operation_type(Type::Invalid)
{
}

ZGPacMessageScaleBarOperation::ZGPacMessageScaleBarOperation(ZInternalID message_id,
                                                             ZInternalID sequence_id,
                                                             ZInternalID gui_id,
                                                             Type operation_type)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageScaleBarOperation>(),
      Util::ClassName<ZGPacMessageScaleBarOperation>(),
      m_sequence_id(sequence_id),
      m_gui_id(gui_id),
      m_operation_type(operation_type)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageScaleBarOperation::ZGPacMessageScaleBarOperation() ; may not set sequence_id to 0 ")) ;
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGPacMessageScaleBarOperation::ZGPacMessageScaleBarOperation() ; may not set gui_id to 0")) ;
    if (m_operation_type == Type::Invalid)
        throw std::runtime_error(std::string("ZGPacMessageScaleBarOperation::ZGPacMessageScaleBarOperation() ; may not set operation_type to Invalid")) ;
}

ZGPacMessageScaleBarOperation::ZGPacMessageScaleBarOperation(const ZGPacMessageScaleBarOperation &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageScaleBarOperation>(),
      Util::ClassName<ZGPacMessageScaleBarOperation>(),
      m_sequence_id(message.m_sequence_id),
      m_gui_id(message.m_gui_id),
      m_operation_type(message.m_operation_type)
{
}

ZGPacMessageScaleBarOperation& ZGPacMessageScaleBarOperation::operator=(const ZGPacMessageScaleBarOperation& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_gui_id = message.m_gui_id ;
        m_operation_type = message.m_operation_type ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageScaleBarOperation::clone() const
{
    return new (std::nothrow) ZGPacMessageScaleBarOperation(*this) ;
}

ZGPacMessageScaleBarOperation::~ZGPacMessageScaleBarOperation()
{
}


} // namespace GUI

} // namespace ZMap




