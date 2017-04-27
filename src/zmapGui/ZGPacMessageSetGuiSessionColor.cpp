#include "ZGPacMessageSetGuiSessionColor.h"

#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageSetGuiSessionColor>::m_name("ZGPacMessageSetGuiSessionColor") ;
const ZGPacMessageType ZGPacMessageSetGuiSessionColor::m_specific_type(ZGPacMessageType::SetGuiSessionColor) ;

ZGPacMessageSetGuiSessionColor::ZGPacMessageSetGuiSessionColor()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageSetGuiSessionColor>(),
      Util::ClassName<ZGPacMessageSetGuiSessionColor>(),
      m_gui_id(0),
      m_color()
{
}

ZGPacMessageSetGuiSessionColor::ZGPacMessageSetGuiSessionColor(ZInternalID message_id,
                                                               ZInternalID gui_id,
                                                               const ZGColor &color)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageSetGuiSessionColor>(),
      Util::ClassName<ZGPacMessageSetGuiSessionColor>(),
      m_gui_id(gui_id),
      m_color(color)
{
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGPacMessageSetGuiSessionColor::ZGPacMessageSetGuiSessionColor() ; may not set gui_id to 0")) ;
}

ZGPacMessageSetGuiSessionColor::ZGPacMessageSetGuiSessionColor(const ZGPacMessageSetGuiSessionColor &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageSetGuiSessionColor>(),
      Util::ClassName<ZGPacMessageSetGuiSessionColor>(),
      m_gui_id(message.m_gui_id),
      m_color(message.m_color)
{
}

ZGPacMessageSetGuiSessionColor& ZGPacMessageSetGuiSessionColor::operator=(const ZGPacMessageSetGuiSessionColor& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_gui_id = message.m_gui_id ;
        m_color = message.m_color ;
    }
    return *this ;
}

ZGPacMessage * ZGPacMessageSetGuiSessionColor::clone() const
{
    return new (std::nothrow) ZGPacMessageSetGuiSessionColor(*this) ;
}

ZGPacMessageSetGuiSessionColor::~ZGPacMessageSetGuiSessionColor()
{
}

} // namespace GUI
} // namespace ZMap
