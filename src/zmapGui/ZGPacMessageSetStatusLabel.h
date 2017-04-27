#ifndef ZGPACMESSAGESETSTATUSLABEL_H
#define ZGPACMESSAGESETSTATUSLABEL_H

#include "ZGPacMessage.h"
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
// Sets the status label and it's tooltip in any gui
// that displays the sequence specified.
//

class ZGPacMessageSetStatusLabel : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageSetStatusLabel>,
        public Util::ClassName<ZGPacMessageSetStatusLabel>
{
public:

    ZGPacMessageSetStatusLabel() ;
    ZGPacMessageSetStatusLabel(ZInternalID message_id,
                               ZInternalID sequence_id,
                               const std::string &label,
                               const std::string &tooltip ) ;
    ZGPacMessageSetStatusLabel(const ZGPacMessageSetStatusLabel& message) ;
    ZGPacMessageSetStatusLabel& operator=(const ZGPacMessageSetStatusLabel& message) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageSetStatusLabel() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    std::string getLabel() const {return m_label ; }
    std::string getTooltip() const {return m_tooltip ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_sequence_id ;
    std::string m_label,
        m_tooltip ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGPACMESSAGESETSTATUSLABEL_H
