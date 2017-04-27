#include "ZGPacMessageSetGuiMenuBarColor.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageSetGuiMenuBarColor>::m_name("ZGPacMessageSetGuiMenuBarColor") ;
const ZGPacMessageType ZGPacMessageSetGuiMenuBarColor::m_specific_type(ZGPacMessageType::SetGuiMenuBarColor) ;


ZGPacMessageSetGuiMenuBarColor::ZGPacMessageSetGuiMenuBarColor()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageSetGuiMenuBarColor>(),
      Util::ClassName<ZGPacMessageSetGuiMenuBarColor>(),
      m_gui_id(0),
      m_color()
{
}

ZGPacMessageSetGuiMenuBarColor::ZGPacMessageSetGuiMenuBarColor(ZInternalID message_id,
                                                               ZInternalID gui_id,
                                                               const ZGColor &color)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageSetGuiMenuBarColor>(),
      Util::ClassName<ZGPacMessageSetGuiMenuBarColor>(),
      m_gui_id(gui_id),
      m_color(color)
{
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGPacMessageSetGuiMenuBarColor::ZGPacMessageSetGuiMenuBarColor() ; may not set gui_id to 0"))  ;
}

ZGPacMessageSetGuiMenuBarColor::ZGPacMessageSetGuiMenuBarColor(const ZGPacMessageSetGuiMenuBarColor& message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageSetGuiMenuBarColor>(),
      Util::ClassName<ZGPacMessageSetGuiMenuBarColor>(),
      m_gui_id(message.m_gui_id),
      m_color(message.m_color)
{
}

ZGPacMessageSetGuiMenuBarColor& ZGPacMessageSetGuiMenuBarColor::operator=(const ZGPacMessageSetGuiMenuBarColor& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator =(message) ;

        m_gui_id = message.m_gui_id ;
        m_color = message.m_color ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageSetGuiMenuBarColor::clone() const
{
    return new(std::nothrow) ZGPacMessageSetGuiMenuBarColor(*this) ;
}

ZGPacMessageSetGuiMenuBarColor::~ZGPacMessageSetGuiMenuBarColor()
{
}


} // namespace GUI
} // namespace ZMap
