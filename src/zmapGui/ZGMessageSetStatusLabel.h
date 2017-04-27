#ifndef ZGMESSAGESETSTATUSLABEL_H
#define ZGMESSAGESETSTATUSLABEL_H

#include "ZGMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{

//
// This sets the status label and it's tooltip in any gui that
// contains the specified sequence.
//

class ZGMessageSetStatusLabel : public ZGMessage,
        public Util::InstanceCounter<ZGMessageSetStatusLabel>,
        public Util::ClassName<ZGMessageSetStatusLabel>
{
public:

    ZGMessageSetStatusLabel();
    ZGMessageSetStatusLabel(ZInternalID message_id,
                            ZInternalID sequence_id,
                            const std::string & label,
                            const std::string & tooltip ) ;
    ZGMessageSetStatusLabel(const ZGMessageSetStatusLabel& message) ;
    ZGMessageSetStatusLabel& operator=(const ZGMessageSetStatusLabel& message) ;
    ~ZGMessageSetStatusLabel() ;
    ZGMessage* clone() const override ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    std::string getLabel() const {return m_label ; }
    std::string getTooltip() const {return m_tooltip ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_sequence_id ;
    std::string m_label,
        m_tooltip ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGMESSAGESETSTATUSLABEL_H
