#include "ZGPacMessageFeatureIDs.h"

#include <new>
#include <stdexcept>

unsigned int ZGPacMessageQueryFeatureIDs::m_instances = 0 ;
const std::string ZGPacMessageQueryFeatureIDs::m_name("ZGPacMessageQueryFeatureIDs") ;
const ZGPacMessageType ZGPacMessageQueryFeatureIDs::m_specific_type(ZGPacMessageType::QueryFeatureIDs) ;

ZGPacMessageQueryFeatureIDs::ZGPacMessageQueryFeatureIDs()
    : ZGPacMessage(m_specific_type),
      m_sequence_id(0),
      m_featureset_id(0)
{
    ++m_instances ;
}

ZGPacMessageQueryFeatureIDs::ZGPacMessageQueryFeatureIDs(ZInternalID message_id, ZInternalID sequence_id, ZInternalID featureset_id)
    : ZGPacMessage(m_specific_type, message_id),
      m_sequence_id(sequence_id),
      m_featureset_id(featureset_id)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageQueryFeatureIDs::ZGPacMessageQueryFeatureIDs() ; sequence_id may not be set to 0 ")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZGPacMessageQueryFeatureIDs::ZGPacMessageQueryFeatureIDs() ; featureset_id may not be set to 0 ")) ;
    ++m_instances ;
}

ZGPacMessageQueryFeatureIDs::ZGPacMessageQueryFeatureIDs(const ZGPacMessageQueryFeatureIDs &message)
    : ZGPacMessage(message),
      m_sequence_id(message.m_sequence_id),
      m_featureset_id(message.m_featureset_id)
{
    ++m_instances ;
}

ZGPacMessageQueryFeatureIDs& ZGPacMessageQueryFeatureIDs::operator=(const ZGPacMessageQueryFeatureIDs& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_featureset_id = message.m_featureset_id ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageQueryFeatureIDs::clone() const
{
    return new (std::nothrow) ZGPacMessageQueryFeatureIDs(*this ) ;
}

ZGPacMessageQueryFeatureIDs::~ZGPacMessageQueryFeatureIDs()
{
    --m_instances ;
}


