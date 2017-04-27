#ifndef ZGPACMESSAGEDELETEALLFEATURES_H
#define ZGPACMESSAGEDELETEALLFEATURES_H

#include "ZGPacMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <string>
#include <cstddef>

namespace ZMap
{

namespace GUI
{

class ZGPacMessageDeleteAllFeatures : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageDeleteAllFeatures>,
        public Util::ClassName<ZGPacMessageDeleteAllFeatures>
{
public:

    ZGPacMessageDeleteAllFeatures();
    ZGPacMessageDeleteAllFeatures(ZInternalID message_id,
                                  ZInternalID sequence_id,
                                  ZInternalID featureset_id) ;
    ZGPacMessageDeleteAllFeatures(const ZGPacMessageDeleteAllFeatures& message) ;
    ZGPacMessageDeleteAllFeatures& operator=(const ZGPacMessageDeleteAllFeatures& message) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageDeleteAllFeatures() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZInternalID getFeaturesetID() const {return m_featureset_id ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_sequence_id,
        m_featureset_id ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGPACMESSAGEDELETEALLFEATURES_H
