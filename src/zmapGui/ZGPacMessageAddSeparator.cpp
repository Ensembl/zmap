#include "ZGPacMessageAddSeparator.h"
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageAddSeparator>::m_name("ZGPacMessageDeleteSeparator") ;
const ZGPacMessageType ZGPacMessageAddSeparator::m_specific_type(ZGPacMessageType::AddSeparator) ;

ZGPacMessageAddSeparator::ZGPacMessageAddSeparator()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageAddSeparator>(),
      Util::ClassName<ZGPacMessageAddSeparator>(),
      m_gui_id(0)
{

}

ZGPacMessageAddSeparator::ZGPacMessageAddSeparator(ZInternalID message_id, ZInternalID gui_id)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageAddSeparator>(),
      Util::ClassName<ZGPacMessageAddSeparator>(),
      m_gui_id(gui_id)
{
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGPacMessageAddSeparator::ZGPacMessageAddSeparator() ; gui_id may not be set to 0")) ;
}


ZGPacMessageAddSeparator::ZGPacMessageAddSeparator(const ZGPacMessageAddSeparator &data)
    : ZGPacMessage(data),
      Util::InstanceCounter<ZGPacMessageAddSeparator>(),
      Util::ClassName<ZGPacMessageAddSeparator>(),
      m_gui_id(data.m_gui_id)
{
}

ZGPacMessageAddSeparator& ZGPacMessageAddSeparator::operator=(const ZGPacMessageAddSeparator& data)
{
    if (this != &data)
    {
        ZGPacMessage::operator=(data) ;

        m_gui_id = data.m_gui_id ;
    }
    return *this ;
}


ZGPacMessageAddSeparator::~ZGPacMessageAddSeparator()
{
}

ZGPacMessage* ZGPacMessageAddSeparator::clone() const
{
    return new (std::nothrow) ZGPacMessageAddSeparator(*this) ;
}


} // namespace GUI

} // namespace ZMap

