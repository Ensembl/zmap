#include "ZGMessageSetGuiMenuBarColor.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageSetGuiMenuBarColor>::m_name("ZGMessageSetGuiMenuBarColor") ;
const ZGMessageType ZGMessageSetGuiMenuBarColor::m_specific_type(ZGMessageType::SetGuiMenuBarColor) ;

ZGMessageSetGuiMenuBarColor::ZGMessageSetGuiMenuBarColor()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageSetGuiMenuBarColor>(),
      Util::ClassName<ZGMessageSetGuiMenuBarColor>(),
      m_gui_id(0),
      m_color()
{
}

ZGMessageSetGuiMenuBarColor::ZGMessageSetGuiMenuBarColor(ZInternalID message_id,
                                                         ZInternalID gui_id,
                                                         const ZGColor &color)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageSetGuiMenuBarColor>(),
      Util::ClassName<ZGMessageSetGuiMenuBarColor>(),
      m_gui_id(gui_id),
      m_color(color)
{
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGMessageSetGuiMenuBarColor::ZGMessageSetGuiMenuBarColor() ; gui_id may not be set to 0")) ;
}

ZGMessageSetGuiMenuBarColor::ZGMessageSetGuiMenuBarColor(const ZGMessageSetGuiMenuBarColor& message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageSetGuiMenuBarColor>(),
      Util::ClassName<ZGMessageSetGuiMenuBarColor>(),
      m_gui_id(message.m_gui_id),
      m_color(message.m_color)
{
}

ZGMessageSetGuiMenuBarColor& ZGMessageSetGuiMenuBarColor::operator =(const ZGMessageSetGuiMenuBarColor& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_gui_id = message.m_gui_id ;
        m_color = message.m_color ;
    }
    return *this ;
}

ZGMessage* ZGMessageSetGuiMenuBarColor::clone() const
{
    return new (std::nothrow) ZGMessageSetGuiMenuBarColor(*this );
}

ZGMessageSetGuiMenuBarColor::~ZGMessageSetGuiMenuBarColor()
{
}


} // namespace GUI
} // namespace ZMap
