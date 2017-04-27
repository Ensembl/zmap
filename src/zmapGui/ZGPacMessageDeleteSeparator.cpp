#include "ZGPacMessageDeleteSeparator.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageDeleteSeparator>::m_name ("ZGPacMessageDeleteSeparator") ;
const ZGPacMessageType ZGPacMessageDeleteSeparator::m_specific_type (ZGPacMessageType::DeleteSeparator) ;

ZGPacMessageDeleteSeparator::ZGPacMessageDeleteSeparator()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageDeleteSeparator>(),
      Util::ClassName<ZGPacMessageDeleteSeparator>(),
      m_gui_id(0)
{
}

ZGPacMessageDeleteSeparator::ZGPacMessageDeleteSeparator(ZInternalID message_id, ZInternalID gui_id)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageDeleteSeparator>(),
      Util::ClassName<ZGPacMessageDeleteSeparator>(),
      m_gui_id(gui_id)
{
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGPacMessageDeleteSeparator::ZGPacMessageDeleteSeparator() ; gui_id may not be set to zero")) ;
}

ZGPacMessageDeleteSeparator::ZGPacMessageDeleteSeparator(const ZGPacMessageDeleteSeparator &data)
    : ZGPacMessage(data),
      Util::InstanceCounter<ZGPacMessageDeleteSeparator>(),
      Util::ClassName<ZGPacMessageDeleteSeparator>(),
      m_gui_id(data.m_gui_id)
{
}

ZGPacMessageDeleteSeparator& ZGPacMessageDeleteSeparator::operator=(const ZGPacMessageDeleteSeparator& data)
{
    if (this != &data)
    {
        ZGPacMessage::operator=(data) ;

        m_gui_id = data.m_gui_id ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageDeleteSeparator::clone() const
{
    return new (std::nothrow) ZGPacMessageDeleteSeparator(*this) ;
}

ZGPacMessageDeleteSeparator::~ZGPacMessageDeleteSeparator()
{
}

} // namespace GUI

} // namespace ZMap

