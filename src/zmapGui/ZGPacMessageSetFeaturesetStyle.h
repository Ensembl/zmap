#ifndef ZGPACMESSAGESETFEATURESETSTYLE_H
#define ZGPACMESSAGESETFEATURESETSTYLE_H

#include "ZGPacMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>
#include <memory>

namespace ZMap
{

namespace GUI
{

class ZGStyle ;

class ZGPacMessageSetFeaturesetStyle : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageSetFeaturesetStyle>,
        public Util::ClassName<ZGPacMessageSetFeaturesetStyle>
{
public:

    ZGPacMessageSetFeaturesetStyle();
    ZGPacMessageSetFeaturesetStyle(ZInternalID message_id,
                                   ZInternalID sequence_id,
                                   ZInternalID featureset_id,
                                   const std::shared_ptr<ZGStyle>& style_data) ;
    ZGPacMessageSetFeaturesetStyle(const ZGPacMessageSetFeaturesetStyle& message) ;
    ZGPacMessageSetFeaturesetStyle& operator = (const ZGPacMessageSetFeaturesetStyle& message) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageSetFeaturesetStyle() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZInternalID getFeaturesetID() const {return m_featureset_id ; }
    std::shared_ptr<ZGStyle> getStyle() const {return m_style_data ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_sequence_id,
        m_featureset_id ;
    std::shared_ptr<ZGStyle> m_style_data ;

};

} // namespace GUI

} // namespace ZMap


#endif // ZGPACMESSAGESETFEATURESETSTYLE_H
