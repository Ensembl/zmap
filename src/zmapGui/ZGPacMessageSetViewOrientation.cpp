#include "ZGPacMessageSetViewOrientation.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageSetViewOrientation>::m_name("ZGPacMessageSetViewOrientation") ;
const ZGPacMessageType ZGPacMessageSetViewOrientation::m_specific_type(ZGPacMessageType::SetViewOrientation) ;

ZGPacMessageSetViewOrientation::ZGPacMessageSetViewOrientation()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageSetViewOrientation>(),
      Util::ClassName<ZGPacMessageSetViewOrientation>(),
      m_sequence_id(0),
      m_gui_id(0),
      m_view_orientation(ZGViewOrientationType::Invalid)
{
}

ZGPacMessageSetViewOrientation::ZGPacMessageSetViewOrientation(ZInternalID message_id,
                                                               ZInternalID sequence_id,
                                                               ZInternalID gui_id,
                                                               ZGViewOrientationType orientation)
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageSetViewOrientation>(),
      Util::ClassName<ZGPacMessageSetViewOrientation>(),
      m_sequence_id(sequence_id),
      m_gui_id(gui_id),
      m_view_orientation(orientation)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageSetViewOrientation::ZGPacMessageSetViewOrientation() ; may not set sequence_id to 0 ")) ;
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGPacMessageSetViewOrientation::ZGPacMessageSetViewOrientation() ; may not set gui_id to 0 ")) ;
    if (m_view_orientation == ZGViewOrientationType::Invalid)
        throw std::runtime_error(std::string("ZGPacMessageSetViewOrientation::ZGPacMessageSetViewOrientation() ; may not set view_orientation to Invalid")) ;
}


ZGPacMessageSetViewOrientation::ZGPacMessageSetViewOrientation(const ZGPacMessageSetViewOrientation &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageSetViewOrientation>(),
      Util::ClassName<ZGPacMessageSetViewOrientation>(),
      m_sequence_id(message.m_sequence_id),
      m_gui_id(message.m_gui_id),
      m_view_orientation(message.m_view_orientation)
{
}

ZGPacMessageSetViewOrientation& ZGPacMessageSetViewOrientation::operator=(const ZGPacMessageSetViewOrientation& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_gui_id = message.m_gui_id ;
        m_view_orientation = message.m_view_orientation ;
    }
    return *this ;
}


ZGPacMessage* ZGPacMessageSetViewOrientation::clone() const
{
    return new (std::nothrow) ZGPacMessageSetViewOrientation(*this) ;
}

ZGPacMessageSetViewOrientation::~ZGPacMessageSetViewOrientation()
{
}

} // namespace GUI

} // namespace ZMap
