#include "ZGMessageQuit.h"
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageQuit>::m_name("ZGMessageQuit") ;
const ZGMessageType ZGMessageQuit::m_specific_type(ZGMessageType::Quit) ;

ZGMessageQuit::ZGMessageQuit()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageQuit>(),
      Util::ClassName<ZGMessageQuit>()
{
}

ZGMessageQuit::ZGMessageQuit(ZInternalID message_id)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageQuit>(),
      Util::ClassName<ZGMessageQuit>()
{
}

ZGMessageQuit::ZGMessageQuit(const ZGMessageQuit& message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageQuit>(),
      Util::ClassName<ZGMessageQuit>()
{
}

ZGMessageQuit& ZGMessageQuit::operator=(const ZGMessageQuit& message)
{
    if (this != &message)
    {
        ZGMessageQuit::operator=(message) ;
    }
    return *this ;
}

ZGMessage* ZGMessageQuit::clone() const
{
    return new (std::nothrow) ZGMessageQuit(*this) ;
}

ZGMessageQuit::~ZGMessageQuit()
{
}

} // namespace GUI

} // namespace ZMap
