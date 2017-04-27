#ifndef ZGMESSAGEDELETEALLFEATURES_H
#define ZGMESSAGEDELETEALLFEATURES_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGMessage.h"
#include <string>
#include <cstddef>

namespace ZMap
{

namespace GUI
{

class ZGMessageDeleteAllFeatures : public ZGMessage,
        public Util::InstanceCounter<ZGMessageDeleteAllFeatures>,
        public Util::ClassName<ZGMessageDeleteAllFeatures>
{
public:

    ZGMessageDeleteAllFeatures();
    ZGMessageDeleteAllFeatures(ZInternalID message_id,
                               ZInternalID sequence_id,
                               ZInternalID featureset_id) ;
    ZGMessageDeleteAllFeatures(const ZGMessageDeleteAllFeatures& message) ;
    ZGMessageDeleteAllFeatures& operator=(const ZGMessageDeleteAllFeatures& message) ;
    ZGMessage* clone() const override ;
    ~ZGMessageDeleteAllFeatures() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZInternalID getFeaturesetID() const {return m_featureset_id ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_sequence_id,
        m_featureset_id ;

};

} // namespace GUI

} // namespace ZMap


#endif // ZGMESSAGEDELETEALLFEATURES_H
