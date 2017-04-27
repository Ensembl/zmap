#include "ZGMessageSetFeaturesetStyle.h"
#include "ZGStyle.h"
#include <stdexcept>
#include <new>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageSetFeaturesetStyle>::m_name("ZGMessageSetFeaturesetStyle") ;
const ZGMessageType ZGMessageSetFeaturesetStyle::m_specific_type(ZGMessageType::SetFeaturesetStyle) ;

ZGMessageSetFeaturesetStyle::ZGMessageSetFeaturesetStyle()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageSetFeaturesetStyle>(),
      Util::ClassName<ZGMessageSetFeaturesetStyle>(),
      m_sequence_id(0),
      m_featureset_id(0),
      m_style_data(0)
{
}

ZGMessageSetFeaturesetStyle::ZGMessageSetFeaturesetStyle(ZInternalID message_id,
                                                         ZInternalID sequence_id,
                                                         ZInternalID featureset_id,
                                                         const std::shared_ptr<ZGStyle> &style_data)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageSetFeaturesetStyle>(),
      Util::ClassName<ZGMessageSetFeaturesetStyle>(),
      m_sequence_id(sequence_id),
      m_featureset_id(featureset_id),
      m_style_data(style_data)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageSetFeaturesetStyle::ZGMessageSetFeaturesetStyle() ; may not set sequence_id to 0 ")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZGMessageSetFeaturesetStyle::ZGMessageSetFeaturesetStyle() ; may not set featureset_id to 0 ")) ;
    if (!m_style_data)
        throw std::runtime_error(std::string("ZGMessageSetFeaturesetStyle::ZGMessageSetFeaturesetStyle() ; may not set style_data to invalid object ")) ;
}

ZGMessageSetFeaturesetStyle::ZGMessageSetFeaturesetStyle(const ZGMessageSetFeaturesetStyle &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageSetFeaturesetStyle>(),
      Util::ClassName<ZGMessageSetFeaturesetStyle>(),
      m_sequence_id(message.m_sequence_id),
      m_featureset_id(message.m_featureset_id),
      m_style_data(message.m_style_data)
{
}

ZGMessageSetFeaturesetStyle& ZGMessageSetFeaturesetStyle::operator=(const ZGMessageSetFeaturesetStyle& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_featureset_id = message.m_featureset_id ;
        m_style_data = message.m_style_data ;
    }
    return *this ;
}

ZGMessage* ZGMessageSetFeaturesetStyle::clone() const
{
    return new (std::nothrow) ZGMessageSetFeaturesetStyle(*this) ;
}

ZGMessageSetFeaturesetStyle::~ZGMessageSetFeaturesetStyle()
{
}

} // namespace GUI

} // namespace ZMap

