#include "ZGMessageDeleteSeparator.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageDeleteSeparator>::m_name ("ZGMessageDeleteSeparator") ;
const ZGMessageType ZGMessageDeleteSeparator::m_specific_type(ZGMessageType::DeleteSeparator) ;

ZGMessageDeleteSeparator::ZGMessageDeleteSeparator()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageDeleteSeparator>(),
      Util::ClassName<ZGMessageDeleteSeparator>(),
      m_gui_id(0)
{
}

ZGMessageDeleteSeparator::ZGMessageDeleteSeparator(ZInternalID message_id, ZInternalID gui_id)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageDeleteSeparator>(),
      Util::ClassName<ZGMessageDeleteSeparator>(),
      m_gui_id(gui_id)
{
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGMessageDeleteSeparator::ZGMessageDeleteSeparator() ; gui id may not be 0")) ;
}

ZGMessageDeleteSeparator::ZGMessageDeleteSeparator(const ZGMessageDeleteSeparator &data)
    : ZGMessage(data),
      Util::InstanceCounter<ZGMessageDeleteSeparator>(),
      Util::ClassName<ZGMessageDeleteSeparator>(),
      m_gui_id(data.m_gui_id)
{
}

ZGMessageDeleteSeparator& ZGMessageDeleteSeparator::operator=(const ZGMessageDeleteSeparator& data)
{
    if (this != &data)
    {
        ZGMessage::operator=(data) ;

        m_gui_id = data.m_gui_id ;
    }
    return *this ;
}

ZGMessage* ZGMessageDeleteSeparator::clone() const
{
    return new (std::nothrow) ZGMessageDeleteSeparator(*this) ;
}

ZGMessageDeleteSeparator::~ZGMessageDeleteSeparator()
{
}

} // namespace GUI

} // namespace ZMap

