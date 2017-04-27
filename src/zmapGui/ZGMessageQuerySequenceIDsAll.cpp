#include "ZGMessageQuerySequenceIDsAll.h"

#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageQuerySequenceIDsAll>::m_name("ZGMessageQuerySequenceIDsAll") ;
const ZGMessageType ZGMessageQuerySequenceIDsAll::m_specific_type(ZGMessageType::QuerySequenceIDsAll) ;

ZGMessageQuerySequenceIDsAll::ZGMessageQuerySequenceIDsAll()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageQuerySequenceIDsAll>(),
      Util::ClassName<ZGMessageQuerySequenceIDsAll>()
{
}

ZGMessageQuerySequenceIDsAll::ZGMessageQuerySequenceIDsAll(ZInternalID message_id)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageQuerySequenceIDsAll>(),
      Util::ClassName<ZGMessageQuerySequenceIDsAll>()
{
}

ZGMessageQuerySequenceIDsAll::ZGMessageQuerySequenceIDsAll(const ZGMessageQuerySequenceIDsAll &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageQuerySequenceIDsAll>(),
      Util::ClassName<ZGMessageQuerySequenceIDsAll>()
{
}

ZGMessageQuerySequenceIDsAll& ZGMessageQuerySequenceIDsAll::operator=(const ZGMessageQuerySequenceIDsAll& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;
    }
    return *this ;
}

ZGMessage* ZGMessageQuerySequenceIDsAll::clone() const
{
    return new (std::nothrow) ZGMessageQuerySequenceIDsAll(*this) ;
}

ZGMessageQuerySequenceIDsAll::~ZGMessageQuerySequenceIDsAll()
{
}

} // namespace GUI

} // namespace ZMap
