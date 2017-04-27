#include "ZGMessageSetGuiSessionColor.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageSetGuiSessionColor>::m_name("ZGMessageSetGuiSessionColor") ;
const ZGMessageType ZGMessageSetGuiSessionColor::m_specific_type(ZGMessageType::SetGuiSessionColor) ;

ZGMessageSetGuiSessionColor::ZGMessageSetGuiSessionColor()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageSetGuiSessionColor>(),
      Util::ClassName<ZGMessageSetGuiSessionColor>(),
      m_gui_id(0),
      m_color()
{
}

ZGMessageSetGuiSessionColor::ZGMessageSetGuiSessionColor(ZInternalID message_id,
                                                         ZInternalID gui_id,
                                                         const ZGColor &color)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageSetGuiSessionColor>(),
      Util::ClassName<ZGMessageSetGuiSessionColor>(),
      m_gui_id(gui_id),
      m_color(color)
{
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGMessageSetGuiSessionColor::ZGMessageSetGuiSessionColor() ; gui_id may not be set to 0")) ;
}

ZGMessageSetGuiSessionColor::ZGMessageSetGuiSessionColor(const ZGMessageSetGuiSessionColor &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageSetGuiSessionColor>(),
      Util::ClassName<ZGMessageSetGuiSessionColor>(),
      m_gui_id(message.m_gui_id),
      m_color(message.m_color)
{
}

ZGMessageSetGuiSessionColor& ZGMessageSetGuiSessionColor::operator=(const ZGMessageSetGuiSessionColor& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_gui_id = message.m_gui_id ;
        m_color = message.m_color ;
    }
    return *this ;
}

ZGMessage* ZGMessageSetGuiSessionColor::clone() const
{
    return new (std::nothrow) ZGMessageSetGuiSessionColor(*this) ;
}

ZGMessageSetGuiSessionColor::~ZGMessageSetGuiSessionColor()
{
}

} // namespace GUI
} // namespace ZMap
