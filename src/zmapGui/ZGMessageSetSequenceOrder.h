#ifndef ZGMESSAGESETSEQUENCEORDER_H
#define ZGMESSAGESETSEQUENCEORDER_H

#include "ZGMessage.h"
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

//
// Set the sequence order in a particular GUI. Order of sequences is specified
// by a container of IDs.
//

class ZGMessageSetSequenceOrder : public ZGMessage,
        public Util::InstanceCounter<ZGMessageSetSequenceOrder>,
        public Util::ClassName<ZGMessageSetSequenceOrder>
{
public:

    ZGMessageSetSequenceOrder();
    ZGMessageSetSequenceOrder(ZInternalID message_id,
                              ZInternalID gui_id,
                              const std::vector<ZInternalID> & data) ;
    ZGMessageSetSequenceOrder(const ZGMessageSetSequenceOrder& message) ;
    ZGMessageSetSequenceOrder& operator=(const ZGMessageSetSequenceOrder& message) ;
    ZGMessage* clone() const override ;
    ~ZGMessageSetSequenceOrder() ;

    ZInternalID getGuiID() const {return m_gui_id ; }
    std::vector<ZInternalID> getSequenceIDs() const {return m_sequence_ids ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_gui_id ;
    std::vector<ZInternalID> m_sequence_ids ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGMESSAGESETSEQUENCEORDER_H
