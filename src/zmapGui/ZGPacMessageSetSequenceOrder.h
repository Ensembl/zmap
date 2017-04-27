#ifndef ZGPACMESSAGESETSEQUENCEORDER_H
#define ZGPACMESSAGESETSEQUENCEORDER_H

#include "ZGPacMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>
#include <vector>

namespace ZMap
{

namespace GUI
{

class ZGPacMessageSetSequenceOrder : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageSetSequenceOrder>,
        public Util::ClassName<ZGPacMessageSetSequenceOrder>
{
public:

    ZGPacMessageSetSequenceOrder();
    ZGPacMessageSetSequenceOrder(ZInternalID message_id,
                                 ZInternalID gui_id,
                                 const std::vector<ZInternalID>& data) ;
    ZGPacMessageSetSequenceOrder(const ZGPacMessageSetSequenceOrder& message) ;
    ZGPacMessageSetSequenceOrder& operator=(const ZGPacMessageSetSequenceOrder& message) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageSetSequenceOrder() ;

    ZInternalID getGuiID() const {return m_gui_id ;}
    std::vector<ZInternalID> getSequenceIDs() const {return m_sequence_ids ;}

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_gui_id ;
    std::vector<ZInternalID> m_sequence_ids ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGPACMESSAGESETSEQUENCEORDER_H
