#include "ZGMessageDeleteAllFeatures.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageDeleteAllFeatures>::m_name ("ZGMessageDeleteAllFeatures") ;
const ZGMessageType ZGMessageDeleteAllFeatures::m_specific_type (ZGMessageType::DeleteAllFeatures) ;

ZGMessageDeleteAllFeatures::ZGMessageDeleteAllFeatures()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageDeleteAllFeatures>(),
      Util::ClassName<ZGMessageDeleteAllFeatures>(),
      m_sequence_id(0),
      m_featureset_id(0)
{
}

ZGMessageDeleteAllFeatures::ZGMessageDeleteAllFeatures(ZInternalID message_id,
                                                       ZInternalID sequence_id,
                                                       ZInternalID featureset_id)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageDeleteAllFeatures>(),
      Util::ClassName<ZGMessageDeleteAllFeatures>(),
      m_sequence_id(sequence_id),
      m_featureset_id(featureset_id)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageDeleteAllFeatures::ZGMessageDeleteAllFeatures() ; sequence_id may not be set to zero")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZGMessageDeleteAllFeatures::ZGMessageDeleteAllFeatures() ; featureset_id may not be set to zero ")) ;
}

ZGMessageDeleteAllFeatures::ZGMessageDeleteAllFeatures(const ZGMessageDeleteAllFeatures& message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageDeleteAllFeatures>(),
      Util::ClassName<ZGMessageDeleteAllFeatures>(),
      m_sequence_id(message.m_sequence_id),
      m_featureset_id(message.m_featureset_id)
{
}

ZGMessageDeleteAllFeatures& ZGMessageDeleteAllFeatures::operator=(const ZGMessageDeleteAllFeatures& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_featureset_id = message.m_featureset_id ;
    }
    return *this ;
}

ZGMessage* ZGMessageDeleteAllFeatures::clone() const
{
    return new (std::nothrow) ZGMessageDeleteAllFeatures(*this) ;
}

ZGMessageDeleteAllFeatures::~ZGMessageDeleteAllFeatures()
{
}

} // namespace GUI

} // namespace ZMap
