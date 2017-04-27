#include "ZGPacMessageQuerySequenceIDsAll.h"

#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageQuerySequenceIDsAll>::m_name("ZGPacMessageQuerySequenceIDsAll") ;
const ZGPacMessageType ZGPacMessageQuerySequenceIDsAll::m_specific_type(ZGPacMessageType::QuerySequenceIDsAll) ;

ZGPacMessageQuerySequenceIDsAll::ZGPacMessageQuerySequenceIDsAll()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageQuerySequenceIDsAll>(),
      Util::ClassName<ZGPacMessageQuerySequenceIDsAll>()
{
}

ZGPacMessageQuerySequenceIDsAll::ZGPacMessageQuerySequenceIDsAll(ZInternalID message_id)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageQuerySequenceIDsAll>(),
      Util::ClassName<ZGPacMessageQuerySequenceIDsAll>()
{
}

ZGPacMessageQuerySequenceIDsAll::ZGPacMessageQuerySequenceIDsAll(const ZGPacMessageQuerySequenceIDsAll &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageQuerySequenceIDsAll>(),
      Util::ClassName<ZGPacMessageQuerySequenceIDsAll>()

{
}

ZGPacMessageQuerySequenceIDsAll& ZGPacMessageQuerySequenceIDsAll::operator=(const ZGPacMessageQuerySequenceIDsAll& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageQuerySequenceIDsAll::clone() const
{
    return new (std::nothrow) ZGPacMessageQuerySequenceIDsAll(*this) ;
}

ZGPacMessageQuerySequenceIDsAll::~ZGPacMessageQuerySequenceIDsAll()
{
}

} // namespace GUI

} // namespace ZMap
