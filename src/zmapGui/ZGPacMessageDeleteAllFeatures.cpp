#include "ZGPacMessageDeleteAllFeatures.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageDeleteAllFeatures>::m_name("ZGPacMessageDeleteAllFeatures") ;
const ZGPacMessageType ZGPacMessageDeleteAllFeatures::m_specific_type(ZGPacMessageType::DeleteAllFeatures) ;

ZGPacMessageDeleteAllFeatures::ZGPacMessageDeleteAllFeatures()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageDeleteAllFeatures>(),
      Util::ClassName<ZGPacMessageDeleteAllFeatures>(),
      m_sequence_id(0),
      m_featureset_id(0)
{
}

ZGPacMessageDeleteAllFeatures::ZGPacMessageDeleteAllFeatures(ZInternalID message_id,
                                                             ZInternalID sequence_id,
                                                             ZInternalID featureset_id)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageDeleteAllFeatures>(),
      Util::ClassName<ZGPacMessageDeleteAllFeatures>(),
      m_sequence_id(sequence_id),
      m_featureset_id(featureset_id)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageDeleteAllFeatures::ZGPacMessageDeleteAllFeatures() ; sequence_id may not be set to zero")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZGPacMessageDeleteAllFeatures::ZGPacMessageDeleteAllFeatures() ; featureset_id may not be set to zero ")) ;
}

ZGPacMessageDeleteAllFeatures::ZGPacMessageDeleteAllFeatures(const ZGPacMessageDeleteAllFeatures& message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageDeleteAllFeatures>(),
      Util::ClassName<ZGPacMessageDeleteAllFeatures>(),
      m_sequence_id(message.m_sequence_id),
      m_featureset_id(message.m_featureset_id)
{
}

ZGPacMessageDeleteAllFeatures& ZGPacMessageDeleteAllFeatures::operator=(const ZGPacMessageDeleteAllFeatures& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_featureset_id = message.m_featureset_id ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageDeleteAllFeatures::clone() const
{
    return new (std::nothrow) ZGPacMessageDeleteAllFeatures(*this) ;
}

ZGPacMessageDeleteAllFeatures::~ZGPacMessageDeleteAllFeatures()
{
}

} // namespace GUI

} // namespace ZMap
