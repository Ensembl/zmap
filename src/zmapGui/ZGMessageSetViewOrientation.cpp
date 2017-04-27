#include "ZGMessageSetViewOrientation.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageSetViewOrientation>::m_name("ZGMessageSetViewOrientation") ;
const ZGMessageType ZGMessageSetViewOrientation::m_specific_type (ZGMessageType::SetViewOrientation) ;

ZGMessageSetViewOrientation::ZGMessageSetViewOrientation()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageSetViewOrientation>(),
      Util::ClassName<ZGMessageSetViewOrientation>(),
      m_sequence_id(0),
      m_gui_id(0),
      m_view_orientation(ZGViewOrientationType::Invalid)
{
}

ZGMessageSetViewOrientation::ZGMessageSetViewOrientation(ZInternalID message_id,
                                                         ZInternalID sequence_id,
                                                         ZInternalID gui_id,
                                                         ZGViewOrientationType orientation)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageSetViewOrientation>(),
      Util::ClassName<ZGMessageSetViewOrientation>(),
      m_sequence_id(sequence_id),
      m_gui_id(gui_id),
      m_view_orientation(orientation)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageSetViewOrientation::ZGMessageSetViewOrientation()  ; may not set sequence_id to 0")) ;
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGMessageSetViewOrientation::ZGMessageSetViewOrientation() ; may not set gui_id to 0 ")) ;
    if (m_view_orientation == ZGViewOrientationType::Invalid)
        throw std::runtime_error(std::string("ZGMessageSetViewOrientation::ZGMessageSetViewOrientation() ; may not set view_orientation to be Invalid ")) ;
}


ZGMessageSetViewOrientation::ZGMessageSetViewOrientation(const ZGMessageSetViewOrientation &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageSetViewOrientation>(),
      Util::ClassName<ZGMessageSetViewOrientation>(),
      m_sequence_id(message.m_sequence_id),
      m_gui_id(message.m_gui_id),
      m_view_orientation(message.m_view_orientation)
{
}

ZGMessageSetViewOrientation& ZGMessageSetViewOrientation::operator=(const ZGMessageSetViewOrientation& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_gui_id = message.m_gui_id ;
        m_view_orientation = message.m_view_orientation ;
    }
    return *this ;
}

ZGMessage* ZGMessageSetViewOrientation::clone() const
{
    return new (std::nothrow) ZGMessageSetViewOrientation(*this) ;
}

ZGMessageSetViewOrientation::~ZGMessageSetViewOrientation()
{
}

} // namespace GUI

} // namespace ZMap

