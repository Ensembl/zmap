#include "ZGPacMessageScaleBarPosition.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageScaleBarPosition>::m_name("ZGPacMessageScaleBarPosition") ;
const ZGPacMessageType ZGPacMessageScaleBarPosition::m_specific_type(ZGPacMessageType::ScaleBarPosition) ;

ZGPacMessageScaleBarPosition::ZGPacMessageScaleBarPosition()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageScaleBarPosition>(),
      Util::ClassName<ZGPacMessageScaleBarPosition>(),
      m_sequence_id(0),
      m_gui_id(0),
      m_scale_position(ZGScaleRelativeType::Invalid)
{
}


ZGPacMessageScaleBarPosition::ZGPacMessageScaleBarPosition(ZInternalID message_id,
                                                           ZInternalID sequence_id,
                                                           ZInternalID gui_id,
                                                           ZGScaleRelativeType position)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageScaleBarPosition>(),
      Util::ClassName<ZGPacMessageScaleBarPosition>(),
      m_sequence_id(sequence_id),
      m_gui_id(gui_id),
      m_scale_position(position)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageScaleBarPosition::ZGPacMessageScaleBarPosition() ; may not set sequence_id to 0")) ;
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGPacMessageScaleBarPosition::ZGPacMessageScaleBarPosition() ; may not set gui_id to 0 ")) ;
    if (m_scale_position == ZGScaleRelativeType::Invalid)
        throw std::runtime_error(std::string("ZGPacMessageScaleBarPosition::ZGPacMessageScaleBarPosition() ; may not set scale_position to Invalid ")) ;
}

ZGPacMessageScaleBarPosition::ZGPacMessageScaleBarPosition(const ZGPacMessageScaleBarPosition& message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageScaleBarPosition>(),
      Util::ClassName<ZGPacMessageScaleBarPosition>(),
      m_sequence_id(message.m_sequence_id),
      m_gui_id(message.m_gui_id),
      m_scale_position(message.m_scale_position)
{
}

ZGPacMessageScaleBarPosition& ZGPacMessageScaleBarPosition::operator=(const ZGPacMessageScaleBarPosition& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_gui_id = message.m_gui_id ;
        m_scale_position = message.m_scale_position ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageScaleBarPosition::clone() const
{
    return new (std::nothrow) ZGPacMessageScaleBarPosition(*this) ;
}

ZGPacMessageScaleBarPosition::~ZGPacMessageScaleBarPosition()
{
}

} // namespace GUI

} // namespace ZMap

