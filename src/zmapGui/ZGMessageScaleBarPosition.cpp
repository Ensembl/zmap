#include "ZGMessageScaleBarPosition.h"
#include <stdexcept>
#include <new>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageScaleBarPosition>::m_name("ZGMessageScaleBarPosition") ;
const ZGMessageType ZGMessageScaleBarPosition::m_specific_type(ZGMessageType::ScaleBarPosition) ;

ZGMessageScaleBarPosition::ZGMessageScaleBarPosition()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageScaleBarPosition>(),
      Util::ClassName<ZGMessageScaleBarPosition>(),
      m_sequence_id(0),
      m_gui_id(0),
      m_scale_position(ZGScaleRelativeType::Invalid)
{
}

ZGMessageScaleBarPosition::ZGMessageScaleBarPosition(ZInternalID message_id,
                                                     ZInternalID sequence_id,
                                                     ZInternalID gui_id,
                                                     ZGScaleRelativeType position)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageScaleBarPosition>(),
      Util::ClassName<ZGMessageScaleBarPosition>(),
      m_sequence_id(sequence_id),
      m_gui_id(gui_id),
      m_scale_position(position)
{

    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageScaleBarPosition::ZGMessageScaleBarPosition() ; may not set sequence_id to 0")) ;
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGMessageScaleBarPosition::ZGMessageScaleBarPosition() ; may not set gui_id to 0 ")) ;
    if (m_scale_position == ZGScaleRelativeType::Invalid)
        throw std::runtime_error(std::string("ZGMessageScaleBarPosition::ZGMessageScaleBarPosition() ; scale_position may not be set to Invalid ")) ;
}


ZGMessageScaleBarPosition::ZGMessageScaleBarPosition(const ZGMessageScaleBarPosition &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageScaleBarPosition>(),
      Util::ClassName<ZGMessageScaleBarPosition>(),
      m_sequence_id(message.m_sequence_id),
      m_gui_id(message.m_gui_id),
      m_scale_position(message.m_scale_position)
{
}

ZGMessageScaleBarPosition& ZGMessageScaleBarPosition::operator=(const ZGMessageScaleBarPosition& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_gui_id = message.m_gui_id ;
        m_scale_position = message.m_scale_position ;
    }
    return *this ;
}

ZGMessage* ZGMessageScaleBarPosition::clone() const
{
    return new (std::nothrow) ZGMessageScaleBarPosition(*this) ;
}

ZGMessageScaleBarPosition::~ZGMessageScaleBarPosition()
{
}

} // namespace GUI

} // namespace ZMap

