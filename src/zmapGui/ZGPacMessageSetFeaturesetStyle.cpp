#include "ZGPacMessageSetFeaturesetStyle.h"
#include "ZGStyle.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageSetFeaturesetStyle>::m_name ("ZGPacMessageSetFeaturesetStyle") ;
const ZGPacMessageType ZGPacMessageSetFeaturesetStyle::m_specific_type(ZGPacMessageType::SetFeaturesetStyle) ;

ZGPacMessageSetFeaturesetStyle::ZGPacMessageSetFeaturesetStyle()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageSetFeaturesetStyle>(),
      Util::ClassName<ZGPacMessageSetFeaturesetStyle>(),
      m_sequence_id(0),
      m_featureset_id(0),
      m_style_data()
{
}

ZGPacMessageSetFeaturesetStyle::ZGPacMessageSetFeaturesetStyle(ZInternalID message_id,
                                                               ZInternalID sequence_id,
                                                               ZInternalID featureset_id,
                                                               const std::shared_ptr<ZGStyle> &style_data)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageSetFeaturesetStyle>(),
      Util::ClassName<ZGPacMessageSetFeaturesetStyle>(),
      m_sequence_id(sequence_id),
      m_featureset_id(featureset_id),
      m_style_data(style_data)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageSetFeaturesetStyle::ZGPacMessageSetFeaturesetStyle() ; may not set sequence_id to 0 ")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZGPacMessageSetFeaturesetStyle::ZGPacMessageSetFeaturesetStyle() ; may not set featureset_id to 0")) ;
    if (!m_style_data)
        throw std::runtime_error(std::string("ZGPacMessageSetFeaturesetStyle::ZGPacMessageSetFeaturesetStyle() ; style_data does not point to valid object ")) ;
}


ZGPacMessageSetFeaturesetStyle::ZGPacMessageSetFeaturesetStyle(const ZGPacMessageSetFeaturesetStyle &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageSetFeaturesetStyle>(),
      Util::ClassName<ZGPacMessageSetFeaturesetStyle>(),
      m_sequence_id(message.m_sequence_id),
      m_featureset_id(message.m_featureset_id),
      m_style_data(message.m_style_data)
{
}

ZGPacMessageSetFeaturesetStyle& ZGPacMessageSetFeaturesetStyle::operator=(const ZGPacMessageSetFeaturesetStyle& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_featureset_id = message.m_featureset_id ;
        m_style_data = message.m_style_data ;
    }
    return *this ;
}


ZGPacMessage* ZGPacMessageSetFeaturesetStyle::clone() const
{
    return new (std::nothrow) ZGPacMessageSetFeaturesetStyle(*this) ;
}

ZGPacMessageSetFeaturesetStyle::~ZGPacMessageSetFeaturesetStyle()
{
}

} // namespace GUI

} // namespace ZMap
