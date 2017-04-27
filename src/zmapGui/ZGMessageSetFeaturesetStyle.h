#ifndef ZGMESSAGESETFEATURESETSTYLE_H
#define ZGMESSAGESETFEATURESETSTYLE_H

#include <string>
#include <memory>
#include <cstddef>
#include "ZInternalID.h"
#include "ZGMessage.h"
#include "InstanceCounter.h"
#include "ClassName.h"


namespace ZMap
{

namespace GUI
{

class ZGStyle ;


class ZGMessageSetFeaturesetStyle : public ZGMessage,
        public Util::InstanceCounter<ZGMessageSetFeaturesetStyle>,
        public Util::ClassName<ZGMessageSetFeaturesetStyle>
{
public:

    ZGMessageSetFeaturesetStyle();
    ZGMessageSetFeaturesetStyle(ZInternalID message_id,
                                ZInternalID sequence_id,
                                ZInternalID featureset_id,
                                const std::shared_ptr<ZGStyle>& style_data) ;
    ZGMessageSetFeaturesetStyle(const ZGMessageSetFeaturesetStyle& message) ;
    ZGMessageSetFeaturesetStyle& operator=(const ZGMessageSetFeaturesetStyle& message) ;
    ZGMessage * clone() const override ;
    ~ZGMessageSetFeaturesetStyle() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZInternalID getFeaturesetID() const {return m_featureset_id ; }
    std::shared_ptr<ZGStyle> getStyle() const {return m_style_data ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_sequence_id,
        m_featureset_id ;
    std::shared_ptr<ZGStyle> m_style_data ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGMESSAGESETFEATURESETSTYLE_H
