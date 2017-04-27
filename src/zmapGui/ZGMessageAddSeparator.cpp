#include "ZGMessageAddSeparator.h"
#include <stdexcept>
#include <new>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageAddSeparator>::m_name("ZGMessageAddSeparator") ;
const ZGMessageType ZGMessageAddSeparator::m_specific_type(ZGMessageType::AddSeparator) ;

ZGMessageAddSeparator::ZGMessageAddSeparator()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageAddSeparator>(),
      Util::ClassName<ZGMessageAddSeparator>(),
      m_gui_id(0)
{
}

ZGMessageAddSeparator::ZGMessageAddSeparator(ZInternalID message_id, ZInternalID gui_id)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageAddSeparator>(),
      Util::ClassName<ZGMessageAddSeparator>(),
      m_gui_id(gui_id)
{
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGMessageAddSeparator::ZGMessageAddSeparator() ; gui_id may not be 0")) ;
}

ZGMessageAddSeparator::ZGMessageAddSeparator(const ZGMessageAddSeparator &data)
    : ZGMessage(data),
      Util::InstanceCounter<ZGMessageAddSeparator>(),
      Util::ClassName<ZGMessageAddSeparator>(),
      m_gui_id(data.m_gui_id)
{
}

ZGMessageAddSeparator& ZGMessageAddSeparator::operator=(const ZGMessageAddSeparator& data)
{
    if (this != &data)
    {
        ZGMessage::operator=(data) ;

        m_gui_id = data.m_gui_id ;
    }
    return *this ;
}

ZGMessage* ZGMessageAddSeparator::clone() const
{
    return new (std::nothrow) ZGMessageAddSeparator(*this) ;
}

ZGMessageAddSeparator::~ZGMessageAddSeparator()
{
}

} // namespace GUI

} // namespace ZMap

